// Copyright 2021 Gamergenic.  See full copyright notice in Spice.h.
// Author: chucknoble@gamergenic.com | https://www.gamergenic.com
// 
// Project page:   https://www.gamergenic.com/project/maxq/
// Documentation:  https://maxq.gamergenic.com/
// GitHub:         https://github.com/Gamergenic1/MaxQ/ 

//------------------------------------------------------------------------------
// SpiceUncooked
// K2 Node Compilation
// See comments in Spice/SpiceK2.h.
//------------------------------------------------------------------------------


#include "K2Node_pack.h"
#include "K2Utilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ToolMenu.h"
#include "K2Node_CallFunction.h"
#include "SpiceK2.h"
#include "SpicePlatformDefs.h"
#include "K2SingleOutputOp.h"
//#include "EdGraphSchema_K2.h"
//#include "KismetCompiler.h"


#define LOCTEXT_NAMESPACE "K2Node_pack"


UK2Node_pack::UK2Node_pack(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITOR
    for (const auto& op : GetSupportedOperations())
    {
        op.CheckClass(USpiceK2::StaticClass());
    }
#endif

    OutputPinName = "Vec";

    DefaultValues.Empty();
    for (int i = 0; i < USpiceK2::vpack_inputs_n; ++i)
    {
        DefaultValues.Add(0.);
    }
}



void UK2Node_pack::AllocateDefaultPins()
{
    Super::AllocateDefaultPins();

    const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

//    for (const auto& PinName : USpiceK2::vpack_inputs)
    for (int i = 0; i < USpiceK2::vpack_inputs_n; ++i)
    {
        const auto& PinName = USpiceK2::vpack_inputs[i];
        auto Pin { CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Real, FName(PinName)) };
        Pin->PinToolTip = TEXT("double precision input scalar");

        if(i < DefaultValues.Num())
        {
            K2Schema->SetPinAutogeneratedDefaultValue(Pin, FString::SanitizeFloat(DefaultValues[i]));
            Pin->DefaultValue = FString::SanitizeFloat(0.);
        }
    }

    auto PinCategory = UEdGraphSchema_K2::PC_Wildcard;
    if (!OperandType.Category.IsNone() && OperandType.Category != UEdGraphSchema_K2::PC_Wildcard)
    {
        PinCategory = OperandType.Category;
    }

    auto OutputPin = CreatePin(EGPD_Output, PinCategory, CreateUniquePinName(OutputPinName));
    if (!OperandType.Category.IsNone() && OperandType.Category != UEdGraphSchema_K2::PC_Wildcard)
    {
        OutputPin->PinType.PinCategory = OperandType.Category;
        OutputPin->PinType.PinSubCategoryObject = OperandType.SubCategoryObject;
        OutputPin->PinType.ContainerType = OperandType.Container;
    }

    OutputPin->PinToolTip = TEXT("packed vector");

    RefreshOperand();
}


void UK2Node_pack::ReconstructNode()
{
    Super::ReconstructNode();
}

void UK2Node_pack::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
    Super::ReallocatePinsDuringReconstruction(OldPins);
}

bool UK2Node_pack::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
    const auto& MyPinType = MyPin->PinType;
    const auto& OtherPinType = OtherPin->PinType;

    if (MyPin->Direction == EEdGraphPinDirection::EGPD_Input)
    {
        bool ConnectionOkay = OtherPinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard || OtherPinType.PinCategory == UEdGraphSchema_K2::PC_Double || OtherPinType.PinCategory == UEdGraphSchema_K2::PC_Real;

        if (!ConnectionOkay)
        {
            OutReason = TEXT("Input must be real");
        }
        return !ConnectionOkay;
    }

    bool FixedType = !OperandType.Category.IsNone() && OperandType.Category != UEdGraphSchema_K2::PC_Wildcard;

    if (FixedType && MyPin->Direction == EEdGraphPinDirection::EGPD_Output)
    {
        if (OperandType.Matches(OtherPinType)) return false;
        if (OtherPinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard) return false;

        OutReason = TEXT("Pin type must match exactly");
        return true; 
    }

    if (MyPinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
    {
        if(OtherPinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard) return false;
        
        auto& SupportedTypes = GetSupportedTypes();
        for (int i = 0; i < SupportedTypes.Num(); ++i)
        {
            const auto& k2type = SupportedTypes[i];

            if (k2type.Matches(OtherPinType))
            {
                return false;
            }
        }
    }

    OutReason = TEXT("Pin connection type not supported");
    return true;
}


void UK2Node_pack::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
    Super::NotifyPinConnectionListChanged(Pin);
}

void UK2Node_pack::NodeConnectionListChanged()
{
    Super::NodeConnectionListChanged();

    bool NodeIsGeneric = true;
    bool bOperandChanged = false;
    FEdGraphPinType FoundType;

    // Determine if there are any connected pins that aren't wildcards.
    // And, if so, what type is it connected to?
    UEdGraphPin** ppOutputPin = Pins.FindByPredicate([](const UEdGraphPin* Pin) -> bool
        {
            return Pin->Direction == EEdGraphPinDirection::EGPD_Output;
        });

    if (!ppOutputPin)
    {
        return;
    }

    UEdGraphPin* OutputPin{ *ppOutputPin };

    for (UEdGraphPin* ConnectedPin : OutputPin->LinkedTo)
    {
        const auto& ConnectedPinType = ConnectedPin->PinType;

        if (ConnectedPinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
        {
            NodeIsGeneric = false;
            FoundType = ConnectedPinType;
            break;
        }
    }

    if (NodeIsGeneric)
    {
        // Switch non-wildcard pin to wildcards.
        SetPinTypeToWildcard(this, OutputPin);
        OperandType = FK2Type::Wildcard();
    }
    else
    {
        // Find what supported operand type is connected...
        for (const auto& op : GetSupportedTypes())
        {
            if (op.Is(FoundType))
            {
                bOperandChanged = !(OperandType == op);
                OperandType = op;
                SetPinType(this, OutputPin, OperandType);
                break;
            }
        }
    }

    RefreshOperand();
}

void UK2Node_pack::PinTypeChanged(UEdGraphPin* Pin)
{
    Super::PinTypeChanged(Pin);

    const auto& PinType { Pin->PinType };

    if (Pin->Direction == EEdGraphPinDirection::EGPD_Output)
    {
        // Update the tooltip
        for (const auto& Type : GetSupportedTypes())
        {
            if (Type.Is(PinType))
            {
                Pin->PinToolTip = FString::Printf(TEXT("Packed Vector (%s)"), *(Type.GetDisplayNameString()));
                break;
            }
        }

        ThisPinTypeChanged(Pin);
    }
}


bool UK2Node_pack::CheckForErrors(FKismetCompilerContext& CompilerContext, OperationType& Operation)
{
    for (const auto& op : GetSupportedOperations())
    {
        if (op.OuterType == OperandType)
        {
            Operation = op;
            return false;
        }
    }

    CompilerContext.MessageLog.Error(*LOCTEXT("Error", "Node @@ had an operand type error.").ToString(), this);
    return true;
}

void UK2Node_pack::Serialize(FArchive& Ar)
{
    Super::Serialize(Ar);
}

void UK2Node_pack::PostLoad()
{
    Super::PostLoad();
}

void UK2Node_pack::PostPasteNode()
{
    Super::PostPasteNode();

//    RestoreStateMachineNode();
}


bool UK2Node_pack::IsActionFilteredOut(FBlueprintActionFilter const& Filter)
{
    return false;
}

FSlateIcon UK2Node_pack::GetIconAndTint(FLinearColor& OutColor) const
{
    OutColor = FColor::Emerald;
    return FSlateIcon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
}

FLinearColor UK2Node_pack::GetNodeTitleColor() const
{
    return NodeBackgroundColor;
}


void UK2Node_pack::RefreshOperand()
{
    if (OperandType.SubCategoryObject.Get())
    {
        PinLabels.Empty();

        PinLabels = FK2Type::GetTypePinLabels(OperandType.SubCategoryObject.Get());
    }
    else
    {
        PinLabels.Empty();

        constexpr int NumberOfInputs = 3;

        for (int i = 0; i < NumberOfInputs; ++i)
        {
            PinLabels.Add(FString(USpiceK2::vpack_inputs[i]));
        }
    }

    bool visibilityChange = false;

    for (int i = 0; i < USpiceK2::vpack_inputs_n; ++i)
    {
        const auto& PinName = FName(USpiceK2::vpack_inputs[i]);
        auto Pin = FindPin(PinName);
        if (i < PinLabels.Num())
        {
#if WITH_EDITORONLY_DATA
            Pin->PinFriendlyName = FText::FromString(PinLabels[i]);
#endif
            visibilityChange |= Pin->bHidden;
            Pin->bHidden = false;
        }
        else
        {
            visibilityChange |= !Pin->bHidden;
            Pin->bHidden = true;
        }
    }

    if (visibilityChange)
    {
        GetGraph()->NotifyGraphChanged();
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
    }
    else
    {
        const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
        K2Schema->ForceVisualizationCacheClear();
    }
}

void UK2Node_pack::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);

    auto Schema = Cast< UEdGraphSchema_K2 >(GetSchema());

    OperationType Operation;

    if (CheckForErrors(CompilerContext, Operation))
    {
        BreakAllNodeLinks();
        return;
    }

    UEdGraphPin** ppOutputPin = Pins.FindByPredicate([](const UEdGraphPin* Pin) -> bool
    {
        return Pin->Direction == EEdGraphPinDirection::EGPD_Output;
    });

    if (!ppOutputPin)
    {
        CompilerContext.MessageLog.Error(*LOCTEXT("Error", "Node @@ had an internal type error.").ToString(), this);
        BreakAllNodeLinks();
        return;
    }

    UEdGraphPin* OutputPin { *ppOutputPin };

    auto InternalNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    InternalNode->FunctionReference.SetExternalMember(Operation.K2NodeName, USpiceK2::StaticClass());
    InternalNode->AllocateDefaultPins();
    CompilerContext.MessageLog.NotifyIntermediateObjectCreation(InternalNode, this);

    uint32 inputCount = 0;
    for(const auto& PinName : USpiceK2::vpack_inputs )
    {
        auto InputPin = FindPin(FName{PinName}, EEdGraphPinDirection::EGPD_Input);
        if (auto InternalPin = InternalNode->FindPin(FName{ PinName }, EEdGraphPinDirection::EGPD_Input))
        {
            MovePinLinksOrCopyDefaults(CompilerContext, InputPin, InternalPin);
            inputCount++;
        }
    }

//    ensure(inputCount == ComponentCounts[Operation.InnerType.TypeName]);

    auto InternalOut = InternalNode->GetReturnValuePin();

    if (!Operation.InnerToOuterConversion.ConversionName.IsNone())
    {
        auto ConversionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

        ConversionNode->FunctionReference.SetExternalMember(Operation.InnerToOuterConversion.ConversionName, USpiceK2::StaticClass());
        ConversionNode->AllocateDefaultPins();
        CompilerContext.MessageLog.NotifyIntermediateObjectCreation(ConversionNode, this);

        auto ConversionValueInput = ConversionNode->FindPinChecked(FName(USpiceK2::conv_input), EEdGraphPinDirection::EGPD_Input);
        auto ConversionOut = ConversionNode->GetReturnValuePin();

        Schema->TryCreateConnection(InternalOut, ConversionValueInput);
        MovePinLinksOrCopyDefaults(CompilerContext, OutputPin, ConversionOut);
    }
    else
    {
        MovePinLinksOrCopyDefaults(CompilerContext, OutputPin, InternalOut);
    }

    BreakAllNodeLinks();
}

void UK2Node_pack::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    RegisterAction(ActionRegistrar, GetClass());
}

FText UK2Node_pack::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    // constexpr bool bUseShortNameForTitle{ true };

    switch (TitleType)
    {
    case ENodeTitleType::FullTitle:
        /** The full title, may be multiple lines. */
        return LOCTEXT("ListViewTitle", "vpack");
        // if (!bUseShortNameForTitle && !OperandType.TypeName.IsNone())
        // {
        //     return FText::FromString(FString::Printf(TEXT("vpack %s"), *OperandType.TypeName.ToString()));
        // }
        // break;
    case ENodeTitleType::MenuTitle:
        /** Menu Title for context menus to be displayed in context menus referencing the node. */
        return LOCTEXT("MenuTitle", "vpack - Pack/Init MaxQ vector");
    case ENodeTitleType::ListView:
        /** More concise, single line title. */
        return LOCTEXT("ListViewTitle", "vpack - Pack vector");
    }

    return LOCTEXT("ShortTitle", "vpack");
}



FText UK2Node_pack::GetMenuCategory() const
{
    return LOCTEXT("Category", "MaxQ|Math|Vector");
}


FText UK2Node_pack::GetKeywords() const
{
    return LOCTEXT("Keywords", "VECTOR");
}


FText UK2Node_pack::GetTooltipText() const
{
   
    FText Tooltip = LOCTEXT("Tooltip", "Pack a MaxQ vector Type.  Support limited wildcards in a single action.");

    FText ListStart = LOCTEXT("ListStart", ":\n");
    FText ListSItemeparator = LOCTEXT("ListItemSeparator", ",\n");
    bool bIsFirstItem = true;
    for (const FK2Type& Type : GetSupportedTypes())
    {
        Tooltip = Tooltip.Join(bIsFirstItem ? ListStart : ListSItemeparator, Tooltip, Type.GetDisplayNameText());
        bIsFirstItem = false;
    }

    return Tooltip;
}

const TArray<FK2Type>& UK2Node_pack::GetSupportedTypes() const
{
    static const TArray<FK2Type> SupportedTypes
    {
        OperationType::GetTypesFromOperations(GetSupportedOperations())
    };

    return SupportedTypes;
}

const TArray<UK2Node_pack::OperationType>& UK2Node_pack::GetSupportedOperations() const
{
    static const TArray<OperationType> SupportedOperations
    {
        OperationType{ "vpack dimensionless vector", USpiceK2::vpack_vector, FK2Type::SDimensionlessVector() },
        OperationType{ "vpack distance vector",  USpiceK2::vpack_vector, FK2Conversion::SDimensionlessVectorToSDistanceVector() },
        OperationType{ "vpack velocity vector",  USpiceK2::vpack_vector, FK2Conversion::SDimensionlessVectorToSVelocityVector() },
        OperationType{ "vpack angular velocity",  USpiceK2::vpack_vector, FK2Conversion::SDimensionlessVectorToSAngularVelocity() },
        OperationType{ "vpack dimensionless state vector",  USpiceK2::vpack_state_vector, FK2Type::SDimensionlessStateVector() },
        OperationType{ "vpack state vector", USpiceK2::vpack_state_vector, FK2Conversion::SDimensionlessStateVectorToSStateVector() }
    };

    return SupportedOperations;
}

#undef LOCTEXT_NAMESPACE
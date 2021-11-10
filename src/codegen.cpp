
#include "node.h"
#include "codegen.h"
#include "parser.hpp"

void CodeGenContext::generateCode(NBlock &root) {
    std::cout << "Generating code..\n";
    
    std::vector<const Type*>argTypes;
    FunctionType *ftype = FunctionType::get(Type::getVoidTy(TheContext),false);
    mainFunction = Function::Create(ftype, GlobalValue::InternalLinkage, "main", module);
    BasicBlock *bblock = BasicBlock::Create(TheContext, "entry", mainFunction, 0);
    
    pushBlock(bblock);
    root.codeGen(*this);
    ReturnInst::Create(TheContext, bblock);
    popBlock();
    
    std::cout << "Code is generated\n";
    
    legacy::PassManager pm;
    pm.add(createPrintModulePass(outs()));
    pm.run(*module);
}

GenericValue CodeGenContext::runCode() {
    std::cout << "Running code\n";
    std::string errorStr;
    
    auto ebuild = new llvm::EngineBuilder( std::unique_ptr<Module>(module) );
    ExecutionEngine * ee = ebuild->
        setErrorStr( &errorStr ).
        setEngineKind( llvm::EngineKind::JIT ).
        create();
    
    ee->finalizeObject();
    std::vector<GenericValue> noargs;
    GenericValue v = ee->runFunction(mainFunction, noargs);
    std::cout << "Code was run...\n";
    return v;
}


static Type *typeOf( const NIdentifier& type ) {
    if(type.name.compare("int") == 0) {
        return Type::getInt64Ty(TheContext);
    }
    
    else if(type.name.compare("double") == 0) {
        return Type::getDoubleTy(TheContext);
    }
    
    return Type::getVoidTy(TheContext);
}

/** Code generation **/
Value* NDouble::codeGen(CodeGenContext &context) {
    std::cout << "Creating double : " << value << '\n';
    return ConstantFP::get(Type::getDoubleTy(TheContext), value);
    
}

Value* NInteger::codeGen(CodeGenContext &context) {
    std::cout << "Creating Integer : " << value << '\n';
    return ConstantFP::get(Type::getInt64Ty(TheContext), value);
}

Value* NIdentifier::codeGen(CodeGenContext &context) {
    std::cout << "Creating identifier reference : " << name << '\n';
    if(context.locals().find(name) == context.locals().end()) {
        std::cerr << "Undeclared Variable : " << name << '\n';
        return NULL;
    }
    return new LoadInst(context.locals()[name]->getType()->getPointerElementType(),context.locals()[name], "", false,                       context.currentBlock());
}

Value* NReturnStatement::codeGen(CodeGenContext& context) {
	std::cout << "Generating return code for " << typeid(expression).name() << '\n';
	Value *returnValue = expression.codeGen(context);
	context.setCurrentReturnValue(returnValue);
	return returnValue;
}

Value* NExternDeclaration::codeGen(CodeGenContext& context) {
    std::vector<Type*> argTypes;
    VariableList::const_iterator it;
    for (it = arguments.begin(); it != arguments.end(); it++) {
        argTypes.push_back(typeOf((**it).type));
    }
    FunctionType *ftype = FunctionType::get(typeOf(type), makeArrayRef(argTypes), false);
    Function *function = Function::Create(ftype, GlobalValue::ExternalLinkage, id.name.c_str(), context.module);
    return function;
}

Value* NMethodCall::codeGen(CodeGenContext& context) {
	Function *function = context.module->getFunction(id.name.c_str());
	if (function == NULL) {
		std::cerr << "No such function : " << id.name << '\n';
	}
	std::vector<Value*> args;
	ExpressionList::const_iterator it;
	for (it = arguments.begin(); it != arguments.end(); it++) {
		args.push_back((**it).codeGen(context));
	}
	CallInst *call = CallInst::Create(function, makeArrayRef(args), "", context.currentBlock());
	std::cout << "Creating method call : " << id.name << '\n';
	return call;
}


Value* NBinaryOperator::codeGen(CodeGenContext &context) {
    std::cout << "Creating Binary Operation : " << op << '\n';
    Instruction::BinaryOps instr;
    switch(op) {
        case TPLUS:     instr = Instruction::Add; goto math;
        case TMINUS:    instr = Instruction::Sub; goto math;
        case TMUL:      instr = Instruction::Mul; goto math;
        case TDIV:      instr = Instruction::SDiv; goto math;
    }
    return NULL;

    math:
        return BinaryOperator::Create(instr, lhs.codeGen(context),
                rhs.codeGen(context), "", context.currentBlock());
    
}

Value* NAssignment::codeGen(CodeGenContext &context) {
    std::cout << "Creating assignment for : " << lhs.name << '\n';
    if(context.locals().find(lhs.name) == context.locals().end()) {
        std::cerr << "Undeclared variable : " << lhs.name << '\n';
        return NULL;
    }
    return new StoreInst(rhs.codeGen(context), context.locals()[lhs.name], false, context.currentBlock());
}

Value* NExpressionStatement::codeGen(CodeGenContext &context) {
    std::cout << "Generating code for : " << typeid(expression).name() << '\n';
    return expression.codeGen(context);
}


Value* NVariableDeclaration::codeGen(CodeGenContext &context) {
    std::cout << "Creating Variable Declaration : " << type.name << ' ' << id.name << '\n';
    AllocaInst *alloc = new AllocaInst(typeOf(type), NULL, id.name.c_str(), context.currentBlock());
    context.locals()[id.name] = alloc;
    
    if(assignmentExpr != NULL) {
        NAssignment assn(id, *assignmentExpr);
        assn.codeGen(context);
    }
    return alloc;
}

Value* NFunctionDeclaration::codeGen(CodeGenContext &context) {
    std::vector<Type*> argTypes;
    VariableList::const_iterator it;
    
    for(it = arguments.begin() ; it != arguments.end() ; it++) {
        argTypes.push_back(typeOf((**it).type));
    }
    
    FunctionType *ftype = FunctionType::get(typeOf(type), makeArrayRef(argTypes), false); 
    Function *function = Function::Create(ftype, GlobalValue::InternalLinkage, id.name.c_str(), context.module);
    BasicBlock *bblock = BasicBlock::Create(TheContext, "entry", function, 0);
    
    context.pushBlock(bblock);
    
    Function::arg_iterator argsValues = function->arg_begin();
    Value *argumentValue;
    
    for(it = arguments.begin() ; it != arguments.end() ; it++) {
        (**it).codeGen(context);
        
        argumentValue = &*argsValues++;
        argumentValue->setName((*it)->id.name.c_str());
        StoreInst *inst = new StoreInst(argumentValue, context.locals()[(*it)->id.name], false, bblock);
    }
    
    block.codeGen(context);
    context.popBlock();
    
    std::cout << "Creating function : " << id.name << '\n';
    return function;
}

Value* NBlock::codeGen(CodeGenContext &context) {
    
    StatementList::const_iterator it;
    Value *last = NULL;
    
    for(it = statements.begin() ; it != statements.end() ; it++) {
        std::cout << "Generating code for " << typeid(**it).name() << '\n';
        last = (**it).codeGen(context);
    }
    return last;
}


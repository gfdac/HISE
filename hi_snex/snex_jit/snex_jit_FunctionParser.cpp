/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace snex {
namespace jit {
using namespace juce;
using namespace asmjit;


snex::jit::BlockParser::StatementPtr FunctionParser::parseStatementBlock()
{
	ScopedPointer<Operations::StatementBlock> b = new Operations::StatementBlock(location);

	pushAnonymousScopeId();

	while (!isEOF() && currentType != JitTokens::closeBrace)
	{
		b->addStatement(parseStatement().get());
	}

	popAnonymousScopeId();

	match(JitTokens::closeBrace);

	return b.release();
}

BlockParser::StatementPtr FunctionParser::parseStatement()
{
	bool isConst = matchIf(JitTokens::const_);

	StatementPtr statement;

	if (matchIf(JitTokens::if_))
	{
		statement = parseIfStatement();
	}
	else if (matchIf(JitTokens::openBrace))
	{
		statement = parseStatementBlock();
		
		matchIf(JitTokens::semicolon);
	}
	else if (matchIf(JitTokens::return_))
	{
		statement = parseReturnStatement();
		match(JitTokens::semicolon);
	}
	else if (matchIf(JitTokens::for_))
	{
		statement = parseLoopStatement();
	}

	else if (matchIfTypeToken())
	{
		statement = parseVariableDefinition(isConst);
		match(JitTokens::semicolon);
	}
	else
	{
		statement = parseAssignment();
		match(JitTokens::semicolon);
	}
		
	return statement;
}

BlockParser::StatementPtr FunctionParser::parseReturnStatement()
{
	ExprPtr rt;

	if (currentType == JitTokens::semicolon)
		return new Operations::ReturnStatement(location, nullptr);
	else
		return new Operations::ReturnStatement(location, parseExpression());

}

snex::jit::BlockParser::StatementPtr FunctionParser::parseVariableDefinition(bool isConst)
{
	if (!matchIfSymbol(isConst))
		location.throwError("Expected symbol");

	auto target = new Operations::VariableReference(location, getCurrentSymbol(true));
	
	match(JitTokens::assign_);
	ExprPtr expr = parseExpression();
	return new Operations::Assignment(location, target, JitTokens::assign_, expr);
}

snex::jit::BlockParser::StatementPtr FunctionParser::parseLoopStatement()
{
	match(JitTokens::openParen);

	match(JitTokens::auto_);

	match(JitTokens::bitwiseAnd);
	match(JitTokens::identifier);

	auto variableId = getCurrentSymbol(true);
	match(JitTokens::colon);
	
	ExprPtr loopBlock = parseExpression();

	match(JitTokens::closeParen);
	
	StatementPtr block = parseStatement();

	return new Operations::BlockLoop(location, variableId, loopBlock.get(), block.get());
}

snex::jit::BlockParser::StatementPtr FunctionParser::parseIfStatement()
{
	match(JitTokens::openParen);

	ExprPtr cond = parseBool();

	match(JitTokens::closeParen);

	StatementPtr trueBranch = parseStatement();

	StatementPtr falseBranch;

	if (matchIf(JitTokens::else_))
	{
		falseBranch = parseStatement();
	}

	return new Operations::IfStatement(location, cond, trueBranch, falseBranch);
}

void FunctionParser::finaliseSyntaxTree(SyntaxTree* tree)
{
	auto lastStatement = tree->getLastStatement().get();

	while (auto bl = dynamic_cast<Operations::StatementBlock*>(lastStatement))
		lastStatement = bl->getLastStatement();

	auto isReturn = [](Operations::Statement* s)
	{
		return dynamic_cast<Operations::ReturnStatement*>(s) != nullptr;
	};

	if (isReturn(lastStatement))
		return;

	if (auto is = dynamic_cast<Operations::IfStatement*>(lastStatement))
	{
		if (!is->hasFalseBranch())
			is->throwError("Not all paths return a value");

		auto lastTrueStatement = is->getTrueBranch();
		auto lastFalseStatement = is->getFalseBranch();

		while (auto bl = dynamic_cast<Operations::StatementBlock*>(lastTrueStatement.get()))
			lastTrueStatement = bl->getLastStatement();
		
		while (auto bl = dynamic_cast<Operations::StatementBlock*>(lastTrueStatement.get()))
			lastTrueStatement = bl->getLastStatement();


		auto lastTrueStatementOK = isReturn(lastTrueStatement);
		auto lastFalseStatementOK = isReturn(lastFalseStatement);
		
		if (lastFalseStatementOK && lastTrueStatementOK)
			return;

		is->getTrueBranch()->addStatement(new Operations::ReturnStatement(location, nullptr));
		is->getFalseBranch()->addStatement(new Operations::ReturnStatement(location, nullptr));
	}

	
	tree->addStatement(new Operations::ReturnStatement(location, nullptr));
}

BlockParser::ExprPtr FunctionParser::createBinaryNode(ExprPtr l, ExprPtr r, TokenType op)
{
	return new Operations::BinaryOp(location, l, r, op);
}


snex::jit::BlockParser::StatementPtr FunctionParser::parseAssignment()
{
	ExprPtr left = parseExpression();

	if (matchIfAssignmentType())
	{
		auto t = currentAssignmentType;
		ExprPtr right = parseExpression();

		if (auto ba = dynamic_cast<Operations::BlockAccess*>(left.get()))
		{
			return new Operations::BlockAssignment(location, left, t, right);
		}
		else
		{
			return new Operations::Assignment(location, left, t, right);
		}
		
	}

	return left;
}

Operations::Expression::Ptr FunctionParser::parseFunctionCall()
{
	ScopedPointer<Operations::FunctionCall> f = new Operations::FunctionCall(location, getCurrentSymbol(false));

	while (!isEOF() && currentType != JitTokens::closeParen)
	{
		f->addStatement(parseExpression());
		matchIf(JitTokens::comma);
	};

	match(JitTokens::closeParen);

	return f.release();
}

Operations::Expression::Ptr FunctionParser::parseExpression()
{
	return parseTernaryOperator();
}

BlockParser::ExprPtr FunctionParser::parseTernaryOperator()
{
	ExprPtr condition = parseBool();

	if (matchIf(JitTokens::question))
	{
		ExprPtr trueBranch = parseExpression();
		match(JitTokens::colon);
		ExprPtr falseBranch = parseExpression();
		return new Operations::TernaryOp(location, condition, trueBranch, falseBranch);
	}

	return condition;
}


BlockParser::ExprPtr FunctionParser::parseBool()
{
	const bool isInverted = matchIf(JitTokens::logicalNot);
	ExprPtr result = parseLogicOperation();

	if (!isInverted)
		return result;
	else
		return new Operations::LogicalNot(location, result);
}


snex::jit::BlockParser::ExprPtr FunctionParser::parseLogicOperation()
{
	ExprPtr left = parseComparation();

	if (matchIf(JitTokens::logicalAnd))
	{
		ExprPtr right = parseLogicOperation();
		return new Operations::BinaryOp(location, left, right, JitTokens::logicalAnd);
	}
	else if (matchIf(JitTokens::logicalOr))
	{
		ExprPtr right = parseLogicOperation();
		return new Operations::BinaryOp(location, left, right, JitTokens::logicalOr);
	}
	else
		return left;
}

BlockParser::ExprPtr FunctionParser::parseComparation()
{
	ExprPtr left = parseSum();

	if (currentType == JitTokens::greaterThan ||
		currentType == JitTokens::greaterThanOrEqual ||
		currentType == JitTokens::lessThan ||
		currentType == JitTokens::lessThanOrEqual ||
		currentType == JitTokens::equals ||
		currentType == JitTokens::notEquals)
	{
		TokenType op = currentType;
		skip();
		ExprPtr right = parseSum();
		return new Operations::Compare(location, left, right, op);
	}
	else
		return left;
}


snex::jit::BlockParser::ExprPtr FunctionParser::parseSum()
{
	ExprPtr left(parseDifference());

	if (currentType == JitTokens::plus)
	{
		TokenType op = currentType;		  skip();
		ExprPtr right(parseSum());
		return createBinaryNode(left, right, op);
	}
	else
		return left;
}


BlockParser::ExprPtr FunctionParser::parseProduct()
{
	ExprPtr left(parseTerm());

	if (currentType == JitTokens::times ||
		currentType == JitTokens::divide ||
		currentType == JitTokens::modulo)
	{
		TokenType op = currentType;		  skip();
		ExprPtr right(parseProduct());
		return createBinaryNode(left, right, op);
	}
	else
		return left;
}



BlockParser::ExprPtr FunctionParser::parseDifference()
{
	ExprPtr left(parseProduct());

	if (currentType == JitTokens::minus)
	{
		TokenType op = currentType;		  skip();
		ExprPtr right(parseDifference());
		return createBinaryNode(left, right, op);
	}
	else
		return left;
}

BlockParser::ExprPtr FunctionParser::parseTerm()
{
	if (matchIf(JitTokens::openParen))
	{
		if (matchIfTypeToken()) 
			return parseCast(currentHnodeType);
		else
		{
			auto result = parseExpression();
			match(JitTokens::closeParen);
			return result;
		}
	}
	else
		return parseUnary();
}


snex::jit::BlockParser::ExprPtr FunctionParser::parseCast(Types::ID type)
{
	match(JitTokens::closeParen);
	ExprPtr source(parseTerm());
	return new Operations::Cast(location, source, type);
}


snex::jit::BlockParser::ExprPtr FunctionParser::parseUnary()
{
	if (currentType == JitTokens::identifier ||
		currentType == JitTokens::literal ||
		currentType == JitTokens::minus ||
		currentType == JitTokens::plusplus ||
		currentType == JitTokens::minusminus)
	{
		return parseFactor();
	}
	else if (matchIf(JitTokens::true_))
	{
		return new Operations::Immediate(location, 1);
	}
	else if (matchIf(JitTokens::false_))
	{
		return new Operations::Immediate(location, 0);
	}
	else if (currentType == JitTokens::logicalNot)
	{
		return parseBool();
	}
	else
	{
		location.throwError("Parsing error");
	}

	return nullptr;
}


BlockParser::ExprPtr FunctionParser::parseFactor()
{
	if (matchIf(JitTokens::plusplus))
	{
		matchIfSymbol(false);
		ExprPtr expr = parseReference();
		return new Operations::Increment(location, expr, true, false);
	}
	if (matchIf(JitTokens::minusminus))
	{
		matchIfSymbol(false);
		ExprPtr expr = parseReference();
		return new Operations::Increment(location, expr, true, true);
	}
	if (matchIf(JitTokens::minus))
	{
		if (currentType == JitTokens::literal)
			return parseLiteral(true);
		else
		{
			ExprPtr expr = parseSymbolOrLiteral();
			return new Operations::Negation(location, expr);
		}
	}
	else
		return parseSymbolOrLiteral();
}


BlockParser::ExprPtr FunctionParser::parseSymbolOrLiteral()
{
	if (matchIfSymbol(false))
	{
		if (matchIf(JitTokens::openParen))
			return parseFunctionCall();
		else
		{
			ExprPtr expr = parseReference();

			if (matchIf(JitTokens::plusplus))
				return new Operations::Increment(location, expr, false, false);
			if (matchIf(JitTokens::minusminus))
				return new Operations::Increment(location, expr, false, true);
			if (matchIf(JitTokens::openBracket))
			{
				ExprPtr idx = parseExpression();

				match(JitTokens::closeBracket);

				return new Operations::BlockAccess(location, expr, idx);
			}

			return expr;
		}
			
	}
	else
		return parseLiteral();
}


BlockParser::ExprPtr FunctionParser::parseReference()
{
	return new Operations::VariableReference(location, getCurrentSymbol(false));
}

BlockParser::ExprPtr FunctionParser::parseLiteral(bool isNegative)
{
	auto v = parseVariableStorageLiteral();

	if (isNegative)
	{
		if (v.getType() == Types::ID::Integer) v = VariableStorage(v.toInt() * -1);
		else if (v.getType() == Types::ID::Float)   v = VariableStorage(v.toFloat() * -1.0f);
		else if (v.getType() == Types::ID::Double)  v = VariableStorage(v.toDouble() * -1.0);
	}

	return new Operations::Immediate(location, v);
}

#if 0

	if (matchIfSymbol())
	{
		ExprPtr left;

		

		if (matchIf(JitTokens::plus))
		{
			ExprPtr right = parseExpression();

			return new Operations::BinaryOp(location, left, right);
		}

		return left;
	}

	if (currentType == JitTokens::literal)
	{
		auto v = parseVariableStorageLiteral();
		return new Operations::Immediate(location, v);
	}

	String expression;

	while (currentType != JitTokens::eof && currentType != JitTokens::semicolon)
	{
		expression << currentType;
		skip();
	}

	return new Operations::DummyExpression(location, expression);
}
#endif


}
}
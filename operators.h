#ifndef OPERATORS_H
#define OPERATORS_H

#include "value.h"
#include "blocks.h"
#include <cmath>
#include <string>

using namespace std;

Value evaluateOperator(BlockType op, const vector<Value>& params)
{
    if (params.empty()) return Value(0.0);

    switch (op)
    {
        // ریاضی پایه
        case OP_ADD:
            if (params.size() >= 2)
                return Value(params[0].asNumber() + params[1].asNumber());
            break;

        case OP_SUBTRACT:
            if (params.size() >= 2)
                return Value(params[0].asNumber() - params[1].asNumber());
            break;

        case OP_MULTIPLY:
            if (params.size() >= 2)
                return Value(params[0].asNumber() * params[1].asNumber());
            break;

        case OP_DIVIDE:
            if (params.size() >= 2) {
                double divisor = params[1].asNumber();
                if (divisor == 0) {
                    cout << "Error: Division by zero!\n";
                    return Value(0.0);
                }
                return Value(params[0].asNumber() / divisor);
            }
            break;

            // مقایسه‌ای
        case OP_EQUAL:
            if (params.size() >= 2) {
                // تساوی برای عدد
                if (params[0].type == VALUE_NUMBER && params[1].type == VALUE_NUMBER)
                    return Value(params[0].asNumber() == params[1].asNumber());
                // تساوی برای رشته
                if (params[0].type == VALUE_STRING && params[1].type == VALUE_STRING)
                    return Value(params[0].asString() == params[1].asString());
                // تساوی برای بولی
                if (params[0].type == VALUE_BOOLEAN && params[1].type == VALUE_BOOLEAN)
                    return Value(params[0].asBoolean() == params[1].asBoolean());
                return Value(false);
            }
            break;

        case OP_LESS_THAN:
            if (params.size() >= 2)
                return Value(params[0].asNumber() < params[1].asNumber());
            break;

        case OP_GREATER_THAN:
            if (params.size() >= 2)
                return Value(params[0].asNumber() > params[1].asNumber());
            break;

            // منطقی
        case OP_NOT:
            return Value(!params[0].asBoolean());

        case OP_OR:
            if (params.size() >= 2)
                return Value(params[0].asBoolean() || params[1].asBoolean());
            break;

        case OP_AND:
            if (params.size() >= 2)
                return Value(params[0].asBoolean() && params[1].asBoolean());
            break;

            // رشته‌ای
        case OP_JOIN_STRINGS:
            if (params.size() >= 2)
                return Value(params[0].asString() + params[1].asString());
            break;

        case OP_STRING_LENGTH:
            return Value((double)params[0].asString().length());

        case OP_LETTER_OF:
            if (params.size() >= 2) {
                int index = (int)params[0].asNumber() - 1;
                string str = params[1].asString();
                if (index >= 0 && index < (int)str.length())
                    return Value(string(1, str[index]));
                else
                    return Value("");
            }
            break;

            // ریاضی پیشرفته (امتیازی)
        case OP_ABS:
            return Value(abs(params[0].asNumber()));

        case OP_SQRT:
        {
            double x = params[0].asNumber();
            if (x < 0) {
                cout << "Error: Square root of negative number!\n";
                return Value(0.0);
            }
            return Value(sqrt(x));
        }

        case OP_FLOOR:
            return Value(floor(params[0].asNumber()));

        case OP_CEIL:
            return Value(ceil(params[0].asNumber()));

        case OP_SIN:
            return Value(sin(params[0].asNumber()));

        case OP_COS:
            return Value(cos(params[0].asNumber()));

        case OP_MOD:
            if (params.size() >= 2) {
                double divisor = params[1].asNumber();
                if (divisor == 0) {
                    cout << "Error: Modulo by zero!\n";
                    return Value(0.0);
                }
                return Value(fmod(params[0].asNumber(), divisor));
            }
            break;

        case OP_XOR:
            if (params.size() >= 2)
                return Value(params[0].asBoolean() != params[1].asBoolean());
            break;

        default:
            break;
    }

    return Value(0.0);
}

#endif
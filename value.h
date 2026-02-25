#ifndef VALUE_H
#define VALUE_H

#include <string>
#include <iostream>

using namespace std;

enum ValueType
{
    VALUE_NUMBER,
    VALUE_STRING,
    VALUE_BOOLEAN
};

struct Value
{
    ValueType type;
    double numberValue;
    string stringValue;
    bool boolValue;

    Value() : type(VALUE_NUMBER), numberValue(0), boolValue(false) {}

    Value(double d) : type(VALUE_NUMBER), numberValue(d), boolValue(false) {}

    Value(const string& s) : type(VALUE_STRING), stringValue(s), boolValue(false) {}

    Value(bool b) : type(VALUE_BOOLEAN), boolValue(b), numberValue(0) {}

    double asNumber() const {
        if (type == VALUE_NUMBER)
            return numberValue;
        if (type == VALUE_STRING) {
            try {
                return stod(stringValue);
            } catch (...) {
                return 0.0;
            }
        }
        if (type == VALUE_BOOLEAN)
            return boolValue ? 1.0 : 0.0;
        return 0.0;
    }

    string asString() const {
        if (type == VALUE_STRING)
            return stringValue;
        if (type == VALUE_NUMBER)
            return to_string(numberValue);
        if (type == VALUE_BOOLEAN)
            return boolValue ? "true" : "false";
        return "";
    }

    bool asBoolean() const {
        if (type == VALUE_BOOLEAN)
            return boolValue;
        if (type == VALUE_NUMBER)
            return numberValue != 0.0;
        if (type == VALUE_STRING)
            return !stringValue.empty();
        return false;
    }
};

#endif
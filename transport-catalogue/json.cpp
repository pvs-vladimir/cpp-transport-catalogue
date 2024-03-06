#include "json.h"

#include <iterator>
#include <stdexcept>

namespace json {

using namespace std::literals;

namespace detail {

Node LoadNode(std::istream& input);
Node LoadString(std::istream& input);

std::string LoadLiteral(std::istream& input) {
    std::string result;
    while (std::isalpha(input.peek())) {
        result.push_back(static_cast<char>(input.get()));
    }
    return result;
}

Node LoadNull(std::istream& input) {
    std::string str = LoadLiteral(input);
    if (str == "null"s) {
        return Node(nullptr);
    } else {
        throw ParsingError("Null parsing error: "s + str + " is not null"s);
    }
}

Node LoadArray(std::istream& input) {
    Array result;

    char c;
    for (; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }

    if (c != ']') {
        throw ParsingError("Array parsing error: There is no closing character ']'"s);
    }

    return Node(std::move(result));
}

Node LoadDict(std::istream& input) {
    Dict result;

    char c;
    for (; input >> c && c != '}';) {
        if (c == '"') {
            std::string key = LoadString(input).AsString();
            if (input >> c && c == ':') {
                if (result.find(key) != result.end()) {
                    throw ParsingError("Dict parsing error: Duplicate key '"s + key + "' have been found"s);
                }
                result.emplace(std::move(key), LoadNode(input));
            } else {
                throw ParsingError("Dict parsing error: ':' is expected but '"s + c + "' has been found"s);
            }
        } else if (c != ',') {
            throw ParsingError("Dict parsing error: ',' is expected but '"s + c + "' has been found"s);
        }
    }

    if (c != '}') {
        throw ParsingError("Dict parsing error: There is no closing character '}'"s);
    }

    return Node(std::move(result));
}

Node LoadBool(std::istream& input) {
    std::string str = LoadLiteral(input);
    if (str == "true"s) {
        return Node{true};
    } else if (str == "false"s) {
        return Node{false};
    } else {
        throw ParsingError("Bool parsing error: "s + str + " is not bool"s);
    }
}

Node LoadNumber(std::istream& input) {
    std::string num_str;
    auto read_char = [&num_str, &input]() {
        num_str += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Number parsing error: Failed to read from the stream"s);
        }
    };

    auto read_digits = [&input, read_char](){
        if (!std::isdigit(input.peek())) {
            throw ParsingError("Number parsing error: A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    read_digits();

    bool is_int = true;
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    if (char ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            try {
                return Node{std::stoi(num_str)};
            } catch (...) {
                // В случае неудачи, например, при переполнении
                // код ниже попробует преобразовать строку в double
            }
        }
        return Node{std::stod(num_str)};
    } catch (...) {
        throw ParsingError("Number parsing error: Failed to convert "s + num_str + " to number"s);
    }
}

Node LoadString(std::istream& input) {
    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string str;

    while(true) {
        if (it == end) {
            throw ParsingError("String parsing error: There is no closing character \"");
        }
        const char ch = *it;
        if (ch == '"') {
            ++it;
            break;
        } else if (ch == '\\') {
            ++it;
            if (it == end) {
                throw ParsingError("String parsing error: The reading of the escape-sequence is interrupted");
            }
            const char escape_ch = *it;
            switch (escape_ch) {
                case 'n':
                    str.push_back('\n');
                    break;
                case 't':
                    str.push_back('\t');
                    break;
                case 'r':
                    str.push_back('\r');
                    break;
                case '"':
                    str.push_back('\"');
                    break;
                case '\\':
                    str.push_back('\\');
                    break;
                default:
                    throw ParsingError("String parsing error: Unrecognized escape-sequence \\"s + escape_ch);
            }
        } else if (ch == '\n' || ch == '\r') {
            throw ParsingError("String parsing error: Unexpected end of line");
        } else {
            str.push_back(ch);
        }
        ++it;
    }   

    return Node(std::move(str));
}

Node LoadNode(std::istream& input) {
    char c;
    input >> c;

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == 'n') {
        input.putback(c);
        return LoadNull(input);
    } else if (c == 't' || c == 'f') {
        input.putback(c);
        return LoadBool(input);
    } else if (std::isdigit(c) || c == '-') {
        input.putback(c);
        return LoadNumber(input);
    } else {
        throw ParsingError("Load node error: Unrecognized data type starting with "s + c);
    }
}

struct PrintContext {
    std::ostream& out;
    int indent_step = 4;
    int indent = 0;

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    PrintContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }
};

void PrintNode(const Node& node, const PrintContext& context);

std::string SetEscapeSequences(const std::string& str) {
    std::string result;
    for (const char c : str) {
        if (c == '\n') {
            result += "\\n"s;
        } else if (c == '\t') {
            result += "\\t"s;
        } else if (c == '\r') {
            result += "\\r"s;
        } else if (c == '\"') {
            result += "\\\""s;
        } else if (c == '\\') {
            result += "\\\\"s;
        } else {
            result += c;
        }
    }
    return result;
}

void PrintValue(std::nullptr_t, const PrintContext& context) {
    context.out << "null"s;
}

void PrintValue(const Array& value, const PrintContext& context) {
    if (value.size() > 0) {
        bool is_first = true;
        context.out << "["s << std::endl;
        PrintContext indented_context{context.Indented()};
        indented_context.PrintIndent();
        for (const auto& elem : value) {
            if (!is_first) {
                context.out << ", "s << std::endl;
                indented_context.PrintIndent();
            }
            PrintNode(elem, indented_context);
            is_first = false;
        }
        context.out << std::endl;
        context.PrintIndent();
        context.out << "]"s;
    } else {
        context.out << "[]"s;
    }
}

void PrintValue(const Dict& value, const PrintContext& context) {
    if (value.size() > 0) {
        bool is_first = true;
        context.out << "{"s << std::endl;
        PrintContext indented_context{context.Indented()};
        indented_context.PrintIndent();
        for (const auto& elem : value) {
            if (!is_first) {
                context.out << ", "s << std::endl;
                indented_context.PrintIndent();
            }
            PrintNode(elem.first, indented_context);
            context.out << ": "s;
            PrintNode(elem.second, indented_context);
            is_first = false;
        }
        context.out << std::endl;
        context.PrintIndent();
        context.out << "}"s;
    } else {
        context.out << "{}"s;
    }
}

void PrintValue(bool value, const PrintContext& context) {
    context.out << std::boolalpha << value;
}

void PrintValue(int value, const PrintContext& context) {
    context.out << value;
}

void PrintValue(double value, const PrintContext& context) {
    context.out << value;
}

void PrintValue(const std::string& value, const PrintContext& context) {
    context.out << "\""s << SetEscapeSequences(value) << "\""s;
}

void PrintNode(const Node& node, const PrintContext& context) {
    std::visit([&context](const auto& value) {PrintValue(value, context);}, node.GetValue());
}

}  // namespace detail

bool Node::IsInt() const {
    return std::holds_alternative<int>(*this);
}

bool Node::IsDouble() const {
    return IsInt() || IsPureDouble();
}

bool Node::IsPureDouble() const {
    return std::holds_alternative<double>(*this);
}

bool Node::IsBool() const {
    return std::holds_alternative<bool>(*this);
}

bool Node::IsString() const {
    return std::holds_alternative<std::string>(*this);
}

bool Node::IsNull() const {
    return std::holds_alternative<std::nullptr_t>(*this);
}

bool Node::IsArray() const {
    return std::holds_alternative<Array>(*this);
}

bool Node::IsMap() const {
    return std::holds_alternative<Dict>(*this);
}

int Node::AsInt() const {
    if(!IsInt()) {
        throw std::logic_error("Is not int!"s);
    }

    return std::get<int>(*this);
}

bool Node::AsBool() const {
    if (!IsBool()) {
        throw std::logic_error("Is not bool!"s);
    }

    return std::get<bool>(*this);
}

double Node::AsDouble() const {
    if (!IsDouble()) {
        throw std::logic_error("Is not double!"s);
    } else if (IsPureDouble()) {
        return std::get<double>(*this);
    } else {
        return static_cast<double>(std::get<int>(*this));
    }
}

const std::string& Node::AsString() const {
    if (!IsString()) {
        throw std::logic_error("Is not string!"s);
    }

    return std::get<std::string>(*this);
}

const Array& Node::AsArray() const {
    if (!IsArray()) {
        throw std::logic_error("Is not Array!"s);
    }

    return std::get<Array>(*this);
}

const Dict& Node::AsMap() const {
    if (!IsMap()) {
        throw std::logic_error("Is not Dict!"s);
    }

    return std::get<Dict>(*this);
}

const Node::Value& Node::GetValue() const {
    return *this;
}

bool operator==(const Node& lhs, const Node& rhs) {
    return lhs.GetValue() == rhs.GetValue();
}

bool operator!=(const Node& lhs, const Node& rhs) {
    return !(lhs == rhs);
}

Document::Document(Node root)
    : root_(std::move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}
    
bool operator==(const Document& lhs, const Document& rhs) {
    return lhs.GetRoot() == rhs.GetRoot();
}
    
bool operator!=(const Document& lhs, const Document& rhs) {
    return !(lhs == rhs);
}

Document Load(std::istream& input) {
    return Document{detail::LoadNode(input)};
}

void Print(const Document& doc, std::ostream& output) {
    detail::PrintNode(doc.GetRoot(), detail::PrintContext{output});
}

}  // namespace json
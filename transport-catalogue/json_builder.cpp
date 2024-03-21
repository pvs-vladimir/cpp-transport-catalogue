#include "json_builder.h"

namespace json {

using namespace std::literals;

KeyItemContext Builder::Key(std::string key) {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsMap()) {
        throw std::logic_error("Incorrect place for key: "s + key);
    }
    nodes_stack_.emplace_back(&const_cast<Dict&>(nodes_stack_.back()->AsMap())[std::move(key)]);
    is_key_ = true;
    return *this;
}

Builder& Builder::Value(Node value) {
    if (is_empty_) {
        root_ = std::move(value);
        is_empty_ = false;
        return *this;
    } else if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
        auto& p = const_cast<Array&>(nodes_stack_.back()->AsArray());
        p.push_back(value);
        return *this;
    } else if (is_key_) {
        is_key_ = false;
        *nodes_stack_.back() = std::move(value);
        nodes_stack_.pop_back();
        return *this;
    }
    throw std::logic_error("Incorrect place for value"s);
}

DictItemContext Builder::StartDict() {
    if (is_empty_) {
        is_empty_ = false;
        root_ = Dict{};
        nodes_stack_.push_back(&root_);
        return *this;
    } else if (is_key_) {
        is_key_ = false;
        *nodes_stack_.back() = Dict{};
        nodes_stack_.back() = &(*nodes_stack_.back());
        return *this;
    } else if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
        auto& p = const_cast<Array&>(nodes_stack_.back()->AsArray());
        p.push_back(Dict{});
        nodes_stack_.push_back(&p.back());
        return *this;
    }
    throw std::logic_error("Incorrect place to start dict"s);
}

ArrayItemContext Builder::StartArray() {
    if (is_empty_) {
        is_empty_ = false;
        root_ = Array{};
        nodes_stack_.push_back(&root_);
        return *this;
    } else if (is_key_) {
        is_key_ = false;
        *nodes_stack_.back() = Array{};
        nodes_stack_.back() = &(*nodes_stack_.back());
        return *this;
    } else if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
        auto& p = const_cast<Array&>(nodes_stack_.back()->AsArray());
        p.push_back(Array{});
        nodes_stack_.push_back(&p.back());
        return *this;
    }
    throw std::logic_error("Incorrect place to start array"s);
}

Builder& Builder::EndDict() {
    if (!nodes_stack_.empty() && nodes_stack_.back()->IsMap()) {
        nodes_stack_.pop_back();
        return *this;
    }
    throw std::logic_error("Incorrect place to end dict"s);
}

Builder& Builder::EndArray() {
    if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
        nodes_stack_.pop_back();
        return *this;
    }
    throw std::logic_error("Incorrect place to end array"s);
}

json::Node Builder::Build() {
    if (is_empty_) {
        throw std::logic_error("Node is empty"s);
    } else if (!nodes_stack_.empty()) {
        throw std::logic_error("The creation of node is not completed"s);
    }
    return root_;
}

ItemContext::ItemContext(Builder& builder) : builder_(builder) {
}

KeyItemContext ItemContext::Key(std::string key) {
    return builder_.Key(std::move(key));
}

DictItemContext ItemContext::StartDict() {
    return builder_.StartDict();
}

ArrayItemContext ItemContext::StartArray() {
    return builder_.StartArray();
}

Builder& ItemContext::EndDict() {
    return builder_.EndDict();
}

Builder& ItemContext::EndArray() {
    return builder_.EndArray();
}

DictItemContext KeyItemContext::Value(Node value) {
    return builder_.Value(std::move(value));
}

ArrayItemContext ArrayItemContext::Value(Node value) {
    return builder_.Value(std::move(value));
}

} // json
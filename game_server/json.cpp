#include "json.h"

// Serializes a JSONObject to a string in JSON format
std::string SimpleJSON::serialize_object(const JSONObject& obj) {
    std::stringstream ss;
    ss << "{";
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (it != obj.begin()) ss << ", ";
        ss << "\"" << it->first << "\": " << it->second;
    }
    ss << "}";
    return ss.str();
}

// Serializes a JSONArray to a string in JSON format
std::string SimpleJSON::serialize_array(const JSONArray& arr) {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < arr.size(); ++i) {
        if (i != 0) ss << ", ";
        ss << "\"" << arr[i] << "\"";
    }
    ss << "]";
    return ss.str();
}

// Serializes a NestedObject (array of objects)
std::string SimpleJSON::serialize_nested_object(const NestedObject& nested_obj) {
    std::stringstream ss;
    ss << "{";
    for (auto it = nested_obj.begin(); it != nested_obj.end(); ++it) {
        if (it != nested_obj.begin()) ss << ", ";
        ss << "\"" << it->first << "\": " << serialize_array_of_objects(it->second);
    }
    ss << "}";
    return ss.str();
}

// Serializes an array of JSON objects
std::string SimpleJSON::serialize_array_of_objects(const std::vector<std::map<std::string, std::string>>& arr_of_obj) {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < arr_of_obj.size(); ++i) {
        if (i != 0) ss << ", ";
        ss << "{";
        const auto& obj = arr_of_obj[i];
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            if (it != obj.begin()) ss << ", ";
            ss << "\"" << it->first << "\": \"" << it->second << "\"";
        }
        ss << "}";
    }
    ss << "]";
    return ss.str();
}

// Method to serialize a vector of Cards
std::string SimpleJSON::serialize_cards(const std::vector<Card>& hand) {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < hand.size(); ++i) {
        if (i != 0) ss << ", ";
        ss << serialize_card(hand[i]);
    }
    ss << "]";
    return ss.str();
}

// Method to serialize a Card object
std::string SimpleJSON::serialize_card(const Card& card) {
    std::stringstream ss;
    ss << "{";
    ss << "\"suit\": \"" << card.suit << "\", ";
    ss << "\"value\": \"" << card.value << "\"";
    ss << "}";
    return ss.str();
}

void SimpleJSON::deserialize_object(const std::string& str) {
    data_.clear(); // Clear existing data

    size_t start = str.find("{") + 1;  // Find the opening curly brace
    size_t end = str.rfind("}");       // Find the closing curly brace

    if (start == std::string::npos || end == std::string::npos) {
        throw std::invalid_argument("Invalid JSON object");
    }

    std::string content = str.substr(start, end - start);  // Extract content between braces
    std::stringstream ss(content);
    std::string key_value_pair;

    // Split the content by commas to extract key-value pairs
    while (getline(ss, key_value_pair, ',')) {
        // Trim leading and trailing spaces
        key_value_pair = trim(key_value_pair);

        // Find the position of the colon separating key and value
        size_t colon_pos = key_value_pair.find(":");
        if (colon_pos == std::string::npos) continue;  // Skip invalid entries

        // Extract key and value
        std::string key = key_value_pair.substr(0, colon_pos);
        std::string value = key_value_pair.substr(colon_pos + 1);

        // Trim spaces
        key = trim(key);
        value = trim(value);

        // Remove quotes from around key and value
        if (key.front() == '"' && key.back() == '"') {
            key = key.substr(1, key.size() - 2);
        }
        if (value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }
        data_[key] = value;  // Store strings
    }
}

// Helper function to trim whitespace
std::string SimpleJSON::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, (last - first + 1));
}

// Deserializes a string into a JSONArray
SimpleJSON::JSONArray SimpleJSON::deserialize_array(const std::string& str) {
    JSONArray arr;
    size_t start = str.find("[") + 1;
    size_t end = str.rfind("]");
    if (start == std::string::npos || end == std::string::npos) {
        throw std::invalid_argument("Invalid JSON array");
    }

    std::string content = str.substr(start, end - start);
    std::stringstream ss(content);
    std::string element;
    while (getline(ss, element, ',')) {
        size_t element_start = element.find_first_of("\"") + 1;
        size_t element_end = element.find_last_of("\"");
        element = element.substr(element_start, element_end - element_start);
        arr.push_back(element);
    }

    return arr;
}

void SimpleJSON::assign_multiple_hands(const std::map<std::string, std::vector<Card>>& hands) {
    for (const auto& pair : hands) {
        data_[pair.first] = serialize_cards(pair.second);
    }
}

// Assign a vector of Card objects to a key
void SimpleJSON::assign_cards(const std::string& key, const std::vector<Card>& hand) {
    data_[key] = serialize_cards(hand);
}

// Assign a string value directly to a key
void SimpleJSON::assign_string(const std::string& key, const std::string& value) {
    data_[key] = "\"" + value + "\"";
}

// Assign an integer value directly to a key
void SimpleJSON::assign_int(const std::string& key, int value) {
    data_[key] = std::to_string(value);
}

// Assign a Card object to a key
void SimpleJSON::assign_card(const std::string& key, const Card& card) {
    data_[key] = serialize_card(card);
}

// Assign an array of strings to a key
void SimpleJSON::assign_array(const std::string& key, const JSONArray& arr) {
    data_[key] = serialize_array(arr);
}

// Assign a NestedObject to a key
void SimpleJSON::assign_nested_object(const std::string& key, const NestedObject& nested_obj) {
    data_[key] = serialize_nested_object(nested_obj);
}

// Serialize the whole JSON object to string
std::string SimpleJSON::serialize() {
    return serialize_object(data_);
}

// For using the `[]` operator to insert JSON values (strings or arrays)
std::string& SimpleJSON::operator[](const std::string& key) {
    return data_[key];
}

// Const version of operator[] for accessing const objects
const std::string& SimpleJSON::operator[](const std::string& key) const {
    auto it = data_.find(key);
    if (it == data_.end()) {
        throw std::out_of_range("Key not found in SimpleJSON object: " + key);
    }
    return it->second;
}

// Assign a boolean value to a key
void SimpleJSON::assign_bool(const std::string& key, bool value) {
    data_[key] = value ? "true" : "false";  // Store as string "true" or "false"
}

// Convert a vector of Card objects into a vector of JSON-like maps
std::vector<std::map<std::string, std::string>> SimpleJSON::convert_hand_to_nested(const std::vector<Card>& hand) {
    std::vector<std::map<std::string, std::string>> serialized_hand;
    for (const auto& card : hand) {
        serialized_hand.push_back({{"suit", card.suit}, {"value", card.value}});
    }
    return serialized_hand;
}

const SimpleJSON::JSONObject& SimpleJSON::get_data() const {
    return data_;
}

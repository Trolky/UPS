#ifndef SIMPLE_JSON_H
#define SIMPLE_JSON_H

#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <stdexcept>
#include "card.h"  // Include your Card header file

class SimpleJSON {
public:
    using JSONObject = std::map<std::string, std::string>;
    using JSONArray = std::vector<std::string>;
    using NestedObject = std::map<std::string, std::vector<std::map<std::string, std::string>>>;

    // Serializes a JSONObject to a string in JSON format
    static std::string serialize_object(const JSONObject& obj);

    // Serializes a JSONArray to a string in JSON format
    static std::string serialize_array(const JSONArray& arr);

    // Serializes a NestedObject (array of objects)
    static std::string serialize_nested_object(const NestedObject& nested_obj);

    // Serializes an array of JSON objects
    static std::string serialize_array_of_objects(const std::vector<std::map<std::string, std::string>>& arr_of_obj);

    // Method to serialize a vector of Cards
    static std::string serialize_cards(const std::vector<Card>& hand);

    // Method to serialize a Card object
    static std::string serialize_card(const Card& card);

    void deserialize_object(const std::string& str);

    // Helper function to trim whitespace
    static std::string trim(const std::string& str);

    // Deserializes a string into a JSONArray
    static JSONArray deserialize_array(const std::string& str);

    void assign_multiple_hands(const std::map<std::string, std::vector<Card>>& hands);

    // Assign a vector of Card objects to a key
    void assign_cards(const std::string& key, const std::vector<Card>& hand);

    // Assign a string value directly to a key
    void assign_string(const std::string& key, const std::string& value);

    // Assign an integer value directly to a key
    void assign_int(const std::string& key, int value);

    // Assign a Card object to a key
    void assign_card(const std::string& key, const Card& card);

    // Assign an array of strings to a key
    void assign_array(const std::string& key, const JSONArray& arr);

    // Assign a NestedObject to a key
    void assign_nested_object(const std::string& key, const NestedObject& nested_obj);

    // Serialize the whole JSON object to string
    std::string serialize();

    // For using the `[]` operator to insert JSON values (strings or arrays)
    std::string& operator[](const std::string& key);

    // Const version of operator[] for accessing const objects
    const std::string& operator[](const std::string& key) const;

    // Assign a boolean value to a key
    void assign_bool(const std::string& key, bool value);

    // Convert a vector of Card objects into a vector of JSON-like maps
    static std::vector<std::map<std::string, std::string>> convert_hand_to_nested(const std::vector<Card>& hand);

    const JSONObject& get_data() const;

private:
    JSONObject data_;
};

#endif // SIMPLE_JSON_H
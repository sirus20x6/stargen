import json
import os
import subprocess
import importlib.util

def create_proto_file(json_data, message_name="Message"):
    """
    Generates a .proto file based on the given JSON structure.
    """
    proto_content = 'syntax = "proto3";\n\n'
    proto_content += f'message {message_name} {{\n'

    field_types = {
        str: "string",
        float: "float",
        int: "int32",
        bool: "bool",
        list: "repeated",  # Handle lists as repeated fields
        dict: "message"    # Will handle nested messages
    }

    field_number = 1

    def process_field(field_name, value, field_number):
        field_type = field_types.get(type(value))
        if isinstance(value, dict):  # Nested message
            nested_message_name = f"{field_name.capitalize()}"
            process_nested_message(nested_message_name, value)
            return f'  {nested_message_name} {field_name} = {field_number};\n'
        elif isinstance(value, list) and len(value) > 0:
            element_type = field_types.get(type(value[0]), "string")  # Guess element type from the first item
            return f'  {field_type} {element_type} {field_name} = {field_number};\n'
        else:
            return f'  {field_type} {field_name} = {field_number};\n'

    def process_nested_message(nested_message_name, nested_json_data):
        nonlocal proto_content
        proto_content += f'message {nested_message_name} {{\n'
        nested_field_number = 1
        for key, value in nested_json_data.items():
            proto_content += process_field(key, value, nested_field_number)
            nested_field_number += 1
        proto_content += '}\n'

    if isinstance(json_data, dict):
        for key, value in json_data.items():
            proto_content += process_field(key, value, field_number)
            field_number += 1
    elif isinstance(json_data, list):
        # If the root is a list, treat it as a repeated message
        if len(json_data) > 0 and isinstance(json_data[0], dict):
            proto_content += f'  repeated {message_name} items = 1;\n'
            process_nested_message(message_name, json_data[0])
        else:
            print("The root list contains non-dict elements, which is not supported in this version.")
            return None

    proto_content += '}\n'

    proto_filename = f'{message_name.lower()}.proto'

    # Write the .proto content to a file
    with open(proto_filename, 'w') as proto_file:
        proto_file.write(proto_content)

    print(f"Generated {proto_filename}")
    return proto_filename

def compile_proto(proto_filename):
    """
    Compiles the .proto file to generate Python Protobuf classes.
    """
    result = subprocess.run(["protoc", "--python_out=.", proto_filename], capture_output=True, text=True)

    if result.returncode != 0:
        print(f"Failed to compile {proto_filename}:\n{result.stderr}")
    else:
        print(f"Compiled {proto_filename} successfully")

def load_generated_module(proto_filename):
    """
    Loads the generated Python module dynamically using importlib.
    """
    module_name = proto_filename.replace('.proto', '_pb2')
    spec = importlib.util.spec_from_file_location(module_name, f"{module_name}.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

def json_to_protobuf(json_data, message_class):
    """
    Converts JSON data to a Protobuf message based on the compiled .proto file.
    """
    if isinstance(json_data, list):
        protobuf_message = message_class()  # Create an instance of the top-level message
        for item in json_data:
            item_message = protobuf_message.items.add()  # Add each item in the list
            populate_message(item_message, item)
        return protobuf_message
    else:
        protobuf_message = message_class()
        populate_message(protobuf_message, json_data)
        return protobuf_message

def populate_message(protobuf_message, json_data):
    """
    Recursively populates the protobuf message with data from JSON.
    """
    for key, value in json_data.items():
        if isinstance(value, dict):
            nested_message = getattr(protobuf_message, key)
            populate_message(nested_message, value)
        elif isinstance(value, list):
            repeated_field = getattr(protobuf_message, key)
            repeated_field.extend(value)
        else:
            setattr(protobuf_message, key, value)

def main(json_filename):
    # Load the JSON data from the file
    with open(json_filename, 'r') as f:
        json_data = json.load(f)

    # Generate a proto message name from the file name
    message_name = os.path.splitext(os.path.basename(json_filename))[0].capitalize()

    # Step 1: Generate .proto file from JSON structure
    proto_filename = create_proto_file(json_data, message_name)
    if proto_filename is None:
        print("Failed to generate proto file.")
        return

    # Step 2: Compile the .proto file
    compile_proto(proto_filename)

    # Step 3: Load the generated Protobuf class dynamically
    message_module = load_generated_module(proto_filename)
    message_class = getattr(message_module, message_name)

    # Step 4: Convert JSON data to Protobuf message
    protobuf_message = json_to_protobuf(json_data, message_class)

    # Step 5: Serialize the Protobuf message to binary format and save to a file
    protobuf_bin_filename = f'{message_name.lower()}.bin'
    with open(protobuf_bin_filename, 'wb') as f:
        f.write(protobuf_message.SerializeToString())

    print(f"Protobuf binary data saved to {protobuf_bin_filename}")

    # Optionally: Clean up generated proto files
    os.remove(proto_filename)

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 2:
        print("Usage: python json_to_protobuf_with_dynamic_proto.py <json_file>")
        sys.exit(1)

    json_filename = sys.argv[1]
    main(json_filename)

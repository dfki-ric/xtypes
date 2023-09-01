#!/usr/bin/python3

# TODO: This function could become a template in the xtypes generator

import sys
import argparse
import os
import xtypes_py # TODO: Generatable
import xdbi_py
import json


backends = xdbi_py.get_available_backends()


def main():
    parser = argparse.ArgumentParser(description='Interactive CLI to add a simple Xtype to the graph database')
    # xdbi specific args
    parser.add_argument('-b', '--db_backend', help="Database backend to be used", choices=backends, default=backends[0])
    parser.add_argument('-a', '--db_address', help="The url/local path to the db", required=True)
    parser.add_argument('-g', '--db_graph', help="The name of the database graph to be used", required=True)
    # tool specific args
    parser.add_argument('-c', '--classname',  help="The name of the Xtype-derived class to create [%(default)s]", default="xtypes::ComponentModel")
    parser.add_argument('-r', '--relations',  help="Ask for facts/relations to other Xtypes", default=False, const=True, nargs='?')

    args = None
    try:
        args = parser.parse_args()
    except SystemExit:
        sys.exit(1)
    
    successful = True
    print(f"Setting up registry ...")
    registry = xtypes_py.ProjectRegistry()   

    print(f"Accessing database at {args.db_address}")
    dbi = xdbi_py.db_interface_from_config(registry, config={"type":args.db_backend, "address": args.db_address, "graph": args.db_graph}, read_only=False)

    print(f"Creating Xtype of class '{args.classname}'\n")
    xtype = registry.instantiate_from(args.classname)
    
    print("Property creation")
    updated_props = {}
    user_input = None
    
    for k,v in xtype.get_properties().items():
        allowed_values = sorted(xtype.get_allowed_property_values(k), reverse=True)
        if len(allowed_values) != 0:
            for i, value in enumerate(allowed_values):
                print(f"\t{i+1}. {value}")
            option = input(f"Please select a value for '{k}' [1-{len(allowed_values)}] or press ENTER [{v}]: ")
            try:
                option = int(option)
                if option < 1 or option > len(allowed_values):
                    raise ValueError
                user_input = allowed_values[option-1]
            except ValueError:
                print(f"invalid selection, using default value {v}")
                user_input = None   
        else:
            user_input = input(f"Please provide value for '{k}' or press ENTER [{v}]: ")

        if user_input:
            try:
                updated_props[k] = json.loads(user_input)
            except:
                updated_props[k] = json.loads('\"' + user_input + '\"')
        else:
            updated_props[k] = v
        xtype.set_properties(updated_props)

    if args.relations:
        print("")
        print("Relation creation")
        for relation_name, relation_details in xtype.get_relations().items():
            if not hasattr(xtype, f"add_{relation_name}"):
                continue
            add_method = getattr(xtype, f"add_{relation_name}")
            if not callable(add_method):
                continue

            user_input = input(f"Do you want to define facts to the relation '{relation_name}' ({','.join(relation_details.from_classnames)} -> {','.join(relation_details.to_classnames)})? [s: set, e: set empty, u or nothing: skip/leave unknown]: ")
            if user_input.lower() == "s":
                while True:
                    user_input_uri = input(f"Please enter the URI of the fact to add or press ENTER to skip: ")
                    if user_input_uri:
                        other_side = dbi.load(user_input_uri)
                        if not other_side:
                            print(f"Could not find any Xtype with URI '{user_input_uri}'")
                            continue
                        if other_side.get_classname() not in relation_details.to_classnames:
                            print(f"'{other_side.get_classname()}' class is not in the codomain of '{relation_name}'")
                            continue
                        try:
                            add_method(other_side)
                            print(f"Fact added to relation '{relation_name}'")
                        except Exception as e:
                            print(f"Failed to add fact to relation '{relation_name}'")
                            print(e)
                    else:
                        break
            elif user_input.lower() == "e":
                xtype.set_unknown_fact_empty(relation_name)
            else:
                continue

    print("\nXType to be added:")
    print(xtype.export_to_basic_model(), end="\n\n")

    user_input = input("Do you want to proceed? [y/n]: ")
    if user_input.lower() != 'y':
        sys.exit(2)
    print(f"Adding Xtype with URI {xtype.uri()} ...")

    if dbi.add([xtype]):
        print(f"XType {xtype.uri()} added successfully to database.")
    else:
        print("Failed to add xtype to database.")
        sys.exit(1)

    if successful:
        script_name = sys.argv[0]
        print(f"{script_name} executed successfully")


if __name__ == '__main__':
    main()


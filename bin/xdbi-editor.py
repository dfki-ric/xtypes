#!/usr/bin/python3

import sys
import argparse
import os
import xtypes_py # TODO: Generatable
import xdbi_py
import json

def proceed(msg):
    print(msg)
    choice = input("Do you want to proceed or skip? [s: skip, other: proceed]: ")
    choice = choice.lower()
    if choice == "s":
        return False
    return True

def select_class_from(reg):
    print("*** Select class ***")
    known_classes = sorted(list(reg.get_classnames()))
    for i, x in enumerate(known_classes):
        print(f"{i}. {x}")
    selected_class_index = input(f"Which class of XTypes do you want to add/edit? [0-{len(known_classes)-1}]: ")
    if not selected_class_index:
        return None
    selected_class_index = int(selected_class_index)
    if selected_class_index < 0 or selected_class_index >= len(known_classes):
        return None
    return known_classes[selected_class_index]

def edit_properties(xtype):
    print("*** Edit properties ***")
    current_props = xtype.get_properties()
    while 1:
        for k,v in current_props.items():
            print(f" {k} = {v}")
        k = input("Which property do you want to modify? ENTER will skip: ")
        if not k:
            print("Done")
            break
        if k not in current_props:
            print(f"{k} is not a valid key!")
            continue
        allowed_values = sorted(xtype.get_allowed_property_values(k), reverse=True)
        user_input = None   
        if len(allowed_values) > 0:
            for i, value in enumerate(allowed_values):
                print(f"{i}. {value}")
            option = input(f"Please select a value for '{k}' [0-{len(allowed_values)-1}] or press ENTER: ")
            try:
                option = int(option)
                if option < 0 or option >= len(allowed_values):
                    raise ValueError
                user_input = allowed_values[option]
            except ValueError:
                print("Invalid selection, using default value")
        else:
            user_input = input(f"Please provide value for '{k}' or press ENTER: ")
        if user_input:
            try:
                current_props[k] = json.loads(user_input)
            except:
                current_props[k] = json.loads('\"' + user_input + '\"')
    xtype.set_properties(current_props)

def select_target(dbi, classnames):
    all_targets = []
    for classname in classnames:
        all_targets.extend(dbi.find(classname=classname))
    for i, t in enumerate(all_targets):
        print(f" {i}. {t.uri()}")
    choice = input(f"Select a target XType [0-{len(all_targets)-1}]: ")
    if not choice:
        return None
    choice = int(choice)
    if choice < 0 or choice >= len(all_targets):
        return None
    return all_targets[choice]

def select_fact(xtype, relname):
    if xtype.has_facts(relname):
        current_facts = xtype.get_facts(relname)
        for i,f in enumerate(current_facts):
            print(f" {i}. {f.target.uri()} {f.edge_properties}")
        choice = input(f"Select a fact [0-{len(current_facts)-1}]: ")
        if not choice:
            return None
        choice = int(choice)
        if choice < 0 or choice >= len(current_facts):
            return None
        return current_facts[choice]
    return None

def edit_facts(dbi, xtype):
    print("*** Edit facts ***")
    relations = xtype.get_relations()
    while 1:
        for relname in relations:
            print(f" {relname}:")
            if not xtype.has_facts(relname):
                print("UNKNOWN")
            else:
                facts = xtype.get_facts(relname)
                if len(facts) < 1:
                    print("EMPTY")
                for fact in facts:
                    print(f"  - {fact.edge_properties} -> {fact.target.uri()}")
        relname = input("Which relation do you want to modify? ENTER will skip: ")
        if not relname:
            print("Done")
            break
        if relname not in relations:
            print(f"{relname} is not a valid relation name!")
            continue
        choice = input(f"[a: add a new fact, e: edit an existing fact, r: remove a fact, d: delete all facts, u: set unknown facts to empty, ENTER: skip]: ")
        if not choice:
            continue
        choice = choice.lower()
        if choice == "a":
            # Add a new fact
            target = select_target(dbi, relations[relname].to_classnames if xtype.get_relations_dir(relname) else relations[relname].from_classnames)
            if not target:
                continue
            add_method = getattr(xtype, f"add_{relname}")
            if not callable(add_method):
                print("ERROR: Found non-callable add method!")
                continue
            try:
                add_method(target)
                print(f"Fact added")
            except Exception as e:
                print(f"Failed to add fact")
                print(e)
        elif choice == "e":
            # Edit a fact
            to_edit = select_fact(xtype, relname)
            if not to_edit:
                continue
            add_method = getattr(xtype, f"add_{relname}")
            if not callable(add_method):
                print("ERROR: Found non-callable add method!")
                continue
            # Edit edge properties
            current_props = to_edit.edge_properties
            while 1:
                for k,v in current_props.items():
                    print(f" {k} = {v}")
                k = input("Which property do you want to modify? ENTER will skip: ")
                if not k:
                    break
                if k not in current_props:
                    print(f"{k} is not a valid key!")
                    continue
                new_v = input(f"Please provide a new value for {k}: ")
                current_props[k] = type(current_props[k])(new_v)
            try:
                add_method(to_edit.target, current_props)
                print(f"Fact updated")
            except Exception as e:
                print(f"Failed to update fact")
                print(e)
        elif choice == "r":
            # Delete fact
            to_be_removed = select_fact(xtype, relname)
            if not to_be_removed:
                continue
            xtype.remove_fact(relname, to_be_removed.target)
        elif choice == "d":
            # Delete all facts
            if xtype.has_facts(relname):
               current_facts = xtype.get_facts(relname)
               for other,_ in current_facts:
                   xtype.remove_fact(relname, other)
        elif choice == "u":
            # Set facts to empty
            xtype.set_unknown_fact_empty(relname)
        else:
            print("W00t?!")
            continue

def info_of(xtype):
    print("*** XType info ***")
    print("Properties:")
    for k,v in xtype.get_properties().items():
        print(f" {k} = {v}")
    print("Facts:")
    relations = xtype.get_relations()
    for relname in relations:
        print(f" {relname}:")
        if not xtype.has_facts(relname):
            print("UNKNOWN")
        else:
            facts = xtype.get_facts(relname)
            if len(facts) < 1:
                print("EMPTY")
            for fact in facts:
                print(f"  - {fact.edge_properties} -> {fact.target.uri()}")

def main():
    backends = xdbi_py.get_available_backends()

    config = {
            "type": "MultiDbClient",
            "config":
                {
                    "import_servers": [],
                    "main_server":
                        {
                            "type": None,
                            "address": None,
                            "graph": None
                        }
                }
            }

    parser = argparse.ArgumentParser(description='Interactive CLI to add/edit Xtype(s) in an XROCK database')
    # xdbi specific args
    parser.add_argument('-b', '--db_backend', help="Database backend to be used", choices=backends, default=backends[0])
    parser.add_argument('-a', '--db_address', help="The url/local path to the db", required=True)
    parser.add_argument('-g', '--db_graph', help="The name of the database graph to be used", required=True)
    parser.add_argument('-l', '--no_lookup', help="If set, the main server will NOT be used for lookup", action="store_true")
    parser.add_argument('-i', '--import_server', help="Specify an import server for additional lookup (format: <backend>,<address>,<graph>)", action="append", type=str)

    args = None
    try:
        args = parser.parse_args()
    except SystemExit:
        sys.exit(1)

    config["config"]["main_server"]["type"] = args.db_backend
    config["config"]["main_server"]["address"] = args.db_address
    config["config"]["main_server"]["graph"] = args.db_graph
    import_servers = args.import_server
    if import_servers:
        for import_server in import_servers:
            b,a,g = import_server.split(",", 2)
            config["config"]["import_servers"].append( {"type": b.strip(), "address": a.strip(), "graph": g.strip() } )
    if not args.no_lookup:
        config["config"]["import_servers"].append( { "name": "main", "type": args.db_backend, "address": args.db_address, "graph": args.db_graph } )


    # Acesss the database
    registry = xtypes_py.ProjectRegistry()   
    dbi = xdbi_py.db_interface_from_config(registry, config=config, read_only=False)

    print(f"{parser.description}")

    # Editor loop
    while True:
        # Ask the user if he wants to create a new XType or edit an existing one
        main_task = input("Do you want to create a new or edit an existing XType? [c: create, e: edit, d: delete, ENTER: quit]: ")
        if not main_task:
            break
        if main_task == "c":
            # Select class
            selected_class = select_class_from(registry)
            if not selected_class:
                continue
            # Create new
            new_xtype = registry.instantiate_from(selected_class)
            # Edit properties
            if proceed("Up next: Property editing"):
                edit_properties(new_xtype)
            # Edit relations
            if proceed("Up next: Fact editing"):
                edit_facts(dbi, new_xtype)
            # Save or discard
            info_of(new_xtype)
            if proceed(f"Shall the new XType be stored into database?"):
                if dbi.add([new_xtype]):
                    print("Stored :)")
                else:
                    print("Could not store to database :(")
        elif main_task == "e":
            # Select class
            selected_class = select_class_from(registry)
            if not selected_class:
                continue
            # Select xtype to edit
            existing_xtypes = dbi.find(classname=selected_class)
            if len(existing_xtypes) < 1:
                print(f"No XType of class {selected_class} exist in the database")
                continue
            for i, xtype in enumerate(existing_xtypes):
                print(f"{i}. {xtype.uri()}")
            index = input(f"Please select one of the XTypes above to edit by number [0-{len(existing_xtypes)-1}]: ")
            if not index:
                continue
            index = int(index)
            if index < 0 or index >= len(existing_xtypes):
                continue
            selected_xtype = existing_xtypes[index]
            info_of(selected_xtype)
            # Edit properties
            if proceed("Up next: Property editing"):
                edit_properties(selected_xtype)
            # Edit relations
            if proceed("Up next: Fact editing"):
                edit_facts(dbi, selected_xtype)
            # Save or discard
            info_of(selected_xtype)
            if proceed(f"Shall the modified XType be stored into database?"):
                if dbi.update([selected_xtype]):
                    print("Stored :)")
                else:
                    print("Could not store to database :(")
        elif main_task == "d":
            # Select class
            selected_class = select_class_from(registry)
            if not selected_class:
                continue
            # Select xtype to delete
            existing_xtypes = dbi.find(classname=selected_class)
            if len(existing_xtypes) < 1:
                print(f"No XType of class {selected_class} exist in the database")
                continue
            for i, xtype in enumerate(existing_xtypes):
                print(f"{i}. {xtype.uri()}")
            index = input(f"Please select one of the XTypes above to delete by number [0-{len(existing_xtypes)-1}]: ")
            if not index:
                continue
            index = int(index)
            if index < 0 or index >= len(existing_xtypes):
                continue
            selected_xtype = existing_xtypes[index]
            info_of(selected_xtype)
            if proceed(f"Are you sure that you want to delete this XType?"):
                if dbi.remove(selected_xtype.uri()):
                    print("Removed!")
                else:
                    print("Could not remove the selected XType :(")
        else:
            # Start over
            print("W00t?")
            continue

    print("Bye :)")
    sys.exit(0)

if __name__ == '__main__':
    main()

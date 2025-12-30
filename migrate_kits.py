import re
import json

def parse_nanny(file_path):
    with open(file_path, 'r') as f:
        content = f.read()

    race_map = {
        "RACE_HUMAN": "Human",
        "RACE_BARBARIAN": "Barbarian",
        "RACE_DROW": "Drow Elf",
        "RACE_GREY": "Grey Elf",
        "RACE_MOUNTAIN": "Mountain Dwarf",
        "RACE_DUERGAR": "Duergar Dwarf",
        "RACE_HALFLING": "Halfling",
        "RACE_GNOME": "Gnome",
        "RACE_OGRE": "Ogre",
        "RACE_TROLL": "Troll",
        "RACE_ORC": "Orc",
        "RACE_THRIKREEN": "Thri-Kreen",
        "RACE_CENTAUR": "Centaur",
        "RACE_GITHYANKI": "Githyanki",
        "RACE_MINOTAUR": "Minotaur",
        "RACE_GITHZERAI": "Githzerai",
        "RACE_TIEFLING": "Tiefling",
        "RACE_OROG": "Orog",
        "RACE_HALFELF": "Half-Elf",
        "RACE_ILLITHID": "Illithid",
        "RACE_PILLITHID": "Planetbound Illithid",
        "RACE_PHANTOM": "Phantom",
        "RACE_LICH": "Lich",
        "RACE_PDKNIGHT": "Death Knight",
        "RACE_VAMPIRE": "Vampire",
        "RACE_WIGHT": "Wight",
        "RACE_SGIANT": "Storm Giant",
        "RACE_SHADE": "Shade",
        "RACE_REVENANT": "Revenant",
        "RACE_PSBEAST": "Shadow Beast",
        "RACE_GOBLIN": "Goblin",
        "RACE_DRIDER": "Drider",
        "RACE_KOBOLD": "Kobold",
        "RACE_KUOTOA": "Kuo Toa",
        "RACE_FIRBOLG": "Firbolg",
        "RACE_WOODELF": "Wood Elf",
        "RACE_SKELETON": "Skeleton",
        "RACE_HARPY": "Harpy"
    }

    class_map = {
        "CLASS_WARRIOR": "Warrior",
        "CLASS_RANGER": "Ranger",
        "CLASS_PSIONICIST": "Psionicist",
        "CLASS_PALADIN": "Paladin",
        "CLASS_ANTIPALADIN": "Anti-Paladin",
        "CLASS_CLERIC": "Cleric",
        "CLASS_MONK": "Monk",
        "CLASS_DRUID": "Druid",
        "CLASS_SHAMAN": "Shaman",
        "CLASS_SORCERER": "Sorcerer",
        "CLASS_NECROMANCER": "Necromancer",
        "CLASS_CONJURER": "Conjurer",
        "CLASS_ROGUE": "Rogue",
        "CLASS_ASSASSIN": "Assassin",
        "CLASS_MERCENARY": "Mercenary",
        "CLASS_BARD": "Bard",
        "CLASS_THIEF": "Thief",
        "CLASS_WARLOCK": "Warlock",
        "CLASS_MINDFLAYER": "MindFlayer",
        "CLASS_ALCHEMIST": "Alchemist",
        "CLASS_BERSERKER": "Berserker",
        "CLASS_REAVER": "Reaver",
        "CLASS_ILLUSIONIST": "Illusionist",
        "CLASS_BLIGHTER": "Blighter",
        "CLASS_DREADLORD": "Dreadlord",
        "CLASS_ETHERMANCER": "Ethermancer",
        "CLASS_AVENGER": "Avenger",
        "CLASS_THEURGIST": "Theurgist",
        "CLASS_SUMMONER": "Summoner"
    }

    data = {
        "start_items": [393, 393, 393, 393, 393, 458], # The 5 bandages and 458 item
        "room_items": [
            {"room": 29201, "items": [29319]} # The note
        ],
        "global_classes": {},
        "races": {}
    }

    # 1. Parse CREATE_KIT
    pattern = re.compile(r'CREATE_KIT\(\s*(\w+)\s*,\s*(\w+)\s*,\s*PROTECT\s*\(\s*\{([^}]*)\}\s*\)\);', re.MULTILINE | re.DOTALL)
    for match in pattern.finditer(content):
        race_const, class_const, items_str = match.groups()
        if race_const not in race_map: continue
        race_name = race_map[race_const]
        if race_name not in data["races"]: data["races"][race_name] = {"classes": {}}
        items = [int(i.strip()) for i in items_str.split(',') if i.strip().isdigit()]
        if class_const == '0': data["races"][race_name]["basic"] = items
        elif class_const in class_map: data["races"][race_name]["classes"][class_map[class_const]] = items

    # 2. Parse special basics
    for race, p1, p2 in [("Thri-Kreen", "thrikreen_good_eq", "basic_good"), 
                        ("Thri-Kreen", "thrikreen_evil_eq", "basic_evil"),
                        ("Minotaur", "minotaur_good_eq", "basic_good"),
                        ("Minotaur", "minotaur_evil_eq", "basic_evil")]:
        match = re.search(f'static int {p1}\[\] = \{{([^}}]+)\}};', content)
        if match:
            items = [int(i.strip()) for i in match.group(1).split(',') if i.strip().isdigit()]
            if race not in data["races"]: data["races"][race] = {"classes": {}}
            data["races"][race][p2] = items

    # 3. Parse Blighter (Special case)
    blighter_match = re.search(r'static int blighter_stuff\[\] = \{([^}]+)\};', content)
    if blighter_match:
        items = [int(i.strip()) for i in blighter_match.group(1).split(',') if i.strip().isdigit()]
        data["global_classes"]["Blighter"] = items

    # Clean empty
    for race in list(data["races"].keys()):
        if not data["races"][race].get("basic") and not data["races"][race].get("classes") and not data["races"][race].get("basic_good"):
            del data["races"][race]

    with open('lib/etc/newbie_kits.json', 'w') as f:
        json.dump(data, f, indent=2)

if __name__ == "__main__":
    parse_nanny('src/nanny.c')
    print("Successfully generated newbie_kits.json with extras")

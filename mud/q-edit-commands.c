---------------------------------------------------------------------------
verb add rooms 10
verb cmds render room-list
verb tmp room-list
#m |
#m-+----------------
#m |#C Rooms in {room.area.name} {if args}#n(filter: {args}){end if}
#m-+------------------------------------
#m |#n Use 'goto <room>' to change room.
#m |
{each item in room.area}{cols 19}#m | #Y{item.id}{end col}{end each}

---------------------------------------------------------------------------
(((((using imm cmd instead)))))
verb add room 10
verb cmds render room-info
verb tmp room-info

RoomId:  {room.@id}
Name:    {room.name}
Terrain: {room.class.name}
Light:   {room.class.lightOptions[room.light]} ({room.light})
Desc:    {room.desc}

Exits: {if not room.exits}None.{end if}
{each exit in room.exits}  {pad 12}{exit[0]}{end pad} {pad 8}{exit[2].name}{end pad} -> {exit[1].@id}
{end each}

---------------------------------------------------------------------------
verb areas add
verb areas set render self area-list
verb areas tmp area-list
pastet $.verbs.areas.area-list
#m |
#m-+----------------
#m |#C Quintiqua Areas:
#m-+------------------------------------
#m |#n Use 'goto <name> .' to enter an area.
#m |
{each dom in @area}{each area in dom}#m | #Y {pad 30}{dom.id}.{area.id}{end pad}#n {pad 40}{area.@meta.name}{end pad}
{end each}{end each}


[$.lib.cmd.imm.bgs]
-> $.lib.basic.command

cmd_add = "new";
cmd_update = "upd";
cmd_replace = "upd";

bgDir = $.mud.backgrounds
oldDir = $.mud.oldBGs

help = "View and create ascii backgrounds.\n"
       "\n"
       "Backgrounds are used for boss spells and other special occasions.\n"
       "Please do not over-use them to the point that they become boring.\n"
       "\n"
       "bgs all                      Preview all backgrounds.\n"
       "bgs <id>                     Preview matching backgrounds.\n"
       "bgs raw <id>                 Show backgrounds with raw colour codes.\n"
       "bgs new <id>                 Create (paste in) a new background.\n"
       "bgs upd <id>                 Update (paste in) a background.\n"
       "\n"
       "Example: 'bgs new fire-breath' then paste in your ASCII art.\n"

Command(args, me) {
  args = split(trim(args), " ");
  verb = args[0];
  if (!verb) return this.help;
  func = "cmd_" + verb;
  if (stringp(this[func])) func = "cmd_" + this[func]; // aliases.
  if (!functionp(this[func])) func = "cmd_find";
  return this[func](args, me);
}

cmd_new(args, me) {
  id = args[1];
  if (!id) return "Usage: bgs new <id>   Enter 'bgs' for help.";
  if (exists(this.bgDir, id)) return "Background already exists: "+args[1];
  return this.setBG(id, me);
}

cmd_upd(args, me) {
  id = args[1];
  if (!id) return "Usage: bgs upd <id>   Enter 'bgs' for help.";
  if (!exists(this.bgDir, id)) return "Background does not exist: "+args[1];
  return this.setBG(id, me);
}

setBG(id, me) {
  if (!this.ValidId(id)) return this.invalidIdMsg;
  me.Message("Paste in your ascii art now. Enter '.' to finish.");
  // read lines
  bg = [];
  repeat {
    me.RawMessage(">");
    line = read_line(me.loginSocket);
    if (line == ".") break;
    bg += [ line ];
  }
  // trim blank lines at top and bottom.
  while (bg[0] != null && !trim(bg[0])) bg = bg[1..];
  while (bg[-1] != null && !trim(bg[-1])) bg = bg[0..-2];
  if (!bg) return "No lines entered.";
  // set the background.
  this.bgDir[id] = bg;
  // show preview
  return this.cmd_find([id], me);
}

cmd_delete(args, me) {
  id = args[1];
  if (!id) return "Usage: bgs delete <id>   Enter 'bgs' for help.";
  if (!exists(this.bgDir, id)) return "Background does not exist: "+args[1];
  remove(this.bgDir, id);
  me.Message("Removed background: "+id);
  return 1;
}

cmd_find(args, me) {
  pat = args[0];
  rawMode = (pat == "raw");
  if (rawMode) pat = args[1];
  if (!pat || pat == "all") pat = "";
  // find matches.
  s = "";
  items = this.bgDir;
  foreach(name in items) {
    if (search(name, pat) >= 0) {
      item = items[name];
      bg = join(item,"\n");
      if (rawMode) bg = replace(bg,"#","##");
      pad = to_int( (me.cols - length(name) - 4) / 2 );
      dashes = this.RepeatString("-", pad);
      s += "#K"+dashes+" "+name+" "+dashes+"#n\n\n"+bg+"\n\n";
    }
  }
  if (!s) return "No matching backgrounds: "+(pat||"all");
  me.MessageNoWrap(s);
  return 1;
}



----------------------------------------------------------------- ITEM


[$.lib.cmd.imm.item]
-> $.lib.basic.command

usage = "Unknown item command. Enter 'item' for help."

help = "Edit item properties and events.\n"
       "\n"
       "Every item in the mud is made from a template. You must create the template "
       "first, then clone it to make an in-game item. Once you have done this, "
       "changes to the template will be seen on all clones immediately.\n"
       "\n"
       "Item IDs must be unique across the whole mud. Use one or more dotted words. "
       "A good item ID is something like: arkenthal.guard.sword or calydynia.home.ring\n"
       "\n"
       "item find <name>             Find templates by ID or name.\n"
       "item new <id>                Start editing a new template.\n"
       "item edit <id>               Start editing an existing template.\n"
       "item copy <id> <newid>       Make a copy of a template and start editing.\n"
       "item del <id>                Remove an old template from the mud.\n"
       "\n"
       "Once you have started editing an item template, you can use the following "
       "commands to make changes:\n"
       "\n"
       "item show                    Display all of the item properties.\n"
       "item name <text>             Set the name, e.g. 'small steel knife'\n"
       "item noun <words>            Change the nouns and plurals, e.g. 'knife knives'\n"
       "item adj <words>             Change the adjectives, e.g. 'small steel'\n"
       "item verb <name> <commands>  Set commands for a custom verb.\n"
       "item undo                    Show undo history so you can undo a change.\n"
       "\n"
       "Enter a partial command for more help, e.g. 'item verb'\n"
       "\n"

items = $.mud.item

[$.lib.cmd.imm.item]
cmd_nouns = "noun"
cmd_adjs = "adj"
cmd_fi = "find"
cmd_ed = "edit"
cmd_na = "name"
cmd_no = "noun"
cmd_ad = "adj"
cmd_li = "list"
cmd_ls = "list"
cmd_sh = "list"
cmd_show = "list"

[$.lib.cmd.imm.item]
Command(args, me) {
  args = split(trim(args), " ");
  verb = args[0];
  if (!verb) return this.help;
  func = "cmd_" + verb;
  if (stringp(this[func])) func = "cmd_" + this[func]; // aliases.
  if (!functionp(this[func])) return this.usage;
  return this[func](args, me);
}

[$.lib.cmd.imm.item]
cmd_find(args, me) {
  pat = args[1];
  if (!pat || pat == "all") pat = ""; // :)
  pat = lower_case(pat);
  s = "";
  items = this.items;
  foreach(name in items) {
    item = items[name];
    if (search(name, pat) >= 0) {
      s += this.PadString(name,30) + " " + item.name[0..48] + "\n";
    }
  }
  if (!s) s = "No matching item IDs found: "+pat;
  me.RawMessage(s);
  return 1;
}

[$.lib.cmd.imm.item]
cmd_rename(args, me) {
  from = args[1]; to = args[2];
  if (!from || !to) return "Usage: item rename <id> <newid>   Enter 'item' for help.";
  items = this.items;
  if (!exists(items, from)) return "Item ID does not exist: "+from;
  if (exists(items, to)) return "Item ID already exists: "+to;
  move_object(items[from], items, to);
  me.Message("Changed item ID from: " + from + " to: " + to);
  return 1;
}

[$.lib.cmd.imm.item]
cmd_end(args, me) {
  if (!me.editItem) return "You are not editing an item.";
  me.Message("You stop editing item: " + object_name(me.editItem));
  me.editItem = null;
  return 1;
}

[$.lib.cmd.imm.item]
cmd_edit(args, me) {
  id = args[1];
  if (!id) return "Usage: item edit <id>   Enter 'item' for help.";
  item = this.FindItemID(id);
  if (stringp(item)) return item; // errors.

  if (exists(items, id)) {
    item = items[id];
  } else {
  }
  me.editItem = item;
  return this.cmd_list(args, me);
}

[$.lib.cmd.imm.item]
cmd_list(args, me) {
  item = me.editItem;
  if (!item) return "You are not editing an item. Enter 'item' for help.";
  s = "\n#YYou are editing:#W " + object_name(item) + "#n\n"
      "\n"
      "Name:        " + item.name + "\n"
      "Nouns:       " + join(item.nouns, " ") + "\n"
      "Adjectives:  " + join(item.adjectives, " ") + "\n"
      "\nDescription:\n"
      "  " + join(this.Wrap(replace(item.desc,"$","$$"),76,1),"\n  ") + "\n"
      "\n"
      "Events:\n";
  foreach (name in item.events) {
      s += "  " + name + ":\n";
      foreach (line in item.events[name]) {
          s += "    " + replace(line,"$","$$") + "\n";
      }
  }
  me.Message(s);
  return 1;
}

[$.lib.cmd.imm.item]
cmd_noun(args, me) {
  item = me.editItem;
  if (!item) return "You are not editing an item. Enter 'item' for help.";
  s = "\n#YYou are editing:#W "+object_name(item)+"#n\n\n";
  words = this.CleanWords(args, 1);
  if (!words) {
    return s+"The current nouns are: "+join(item.nouns," ")+"\n\nUse 'item noun <words>' to change them.";
  }
  item.nouns = words;
  me.Message(s+"You change the nouns to: "+join(words," "));
  return 1;
}

[$.lib.cmd.imm.item]
cmd_adj(args, me) {
  item = me.editItem;
  if (!item) return "You are not editing an item. Enter 'item' for help.";
  s = "\n#YYou are editing:#W "+object_name(item)+"#n\n\n";
  words = this.CleanWords(args, 1);
  if (!words) {
    return s+"The current adjectives are: "+join(item.adjectives," ")+"\n\nUse 'item adj <words>' to change them.";
  }
  item.adjectives = words;
  me.Message(s+"You change the adjectives to: "+join(words," "));
  return 1;
}

[$.lib.cmd.imm.item]
cmd_name(args, me) {
  item = me.editItem;
  if (!item) return "You are not editing an item. Enter 'item' for help.";
  s = "\n#YYou are editing:#W "+object_name(item)+"#n\n\n";
  words = trim(join(args[1..]," "));
  if (!words) {
    return s+"The current name is: "+item.name+"\n\nUse 'item name <words>' to change it.";
  }
  item.name = words;
  me.Message(s+"You change the name to: "+words);
  return 1;
}


it find caly
it new item items.ring_of_calydynia
it name Ring of Calydynia
it noun ring rings
it adj small silver ornate
it verb rub echo As you rub the ring, it begins to glow with warmth.; wait 2; echo You feel yourself pulled to another location.; move player calydynia.city.pub1; act $n appears in a warm glow of light.
it

mob new mobs.dw.gaspode
mob name Gaspode the Wonder Dog
mob noun gaspode dog
mob adj scruffy dirty
mob desc He is just a scruffy, dirty dog.
mob wander on
mob unique on
mob add act You become aware of a strange smell, a bit like a very old and slightly damp nursery rug.
mob add act Someone begins to play a harmonica. There seems to be a tune, but most of the notes are wrong.
mob add say Woof bloody woof.
mob add say I expect you're wondering, how come I'm talking.
mob add emote industriously scratches one ear with his hind leg.
mob rem act You become
mop verb pat emote growls threateningly.; wait 2; say How would you like it if someone patted you?
mob


----------------------------------------------------------------- ROOM

pastet $.lib.cmd.imm.room.room-info

#YYou are in: #W{room.@id}   #K(use 'room help' for help)#n

Name:     {room.name}
Light:    {room.class.lightOptions[room.light]} ({room.light})
Terrain:  {room.class.name}

Description:
  {room.desc}

{if room.exits}Exits:{end if}{if not room.exits}No Exits.{end if}
{each exit in room.exits}  {pad 12}{exit[0]}{end pad} {pad 8}{exit[2].col}{exit[2].id}{end pad}#n -> {exit[1].@id}
{end each}

pastet $.lib.cmd.imm.room.room-types

{each type in types}{cols 12}{type.col}{type.id}{end cols}#n{end each}


[$.lib.cmd.imm.room]
-> $.lib.basic.command

usage = "Unknown room command. Enter 'room help' for help."

[$.lib.cmd.imm.room]
help = "Edit the room you are in.\n"
       "\n"
       "Enter a partial command for more help, e.g. 'room terrain'\n"
       "You can abbreviate most commands to a few letters.\n"
       "\n"
       "room                         Display room properties.\n"
       "room name <text>             Set the name/title of the room.\n"
       "room desc <text>             Set the description for the room.\n"
       "room terrain <name>          Set the kind of terrain in the room.\n"
       "room light <num>             Set the light level in the room.\n"
       "\n"
       "room undo name/desc/etc      Undo the last change to a property.\n"
       "room multi <id,> <command>   Modify multiple rooms at once.\n"
       "room setid <id>              Change the room-id of the room.\n"
       "room delete <id>             Delete a room.\n"
       "\n"
       "See also: exits, extras, rooms.\n"

[$.lib.cmd.imm.room]
Command(args, me) {
  args = split(args," ",0,1);
  verb = args[0];
  if (!verb) verb = "show";
  func = "cmd_" + verb;
  if (stringp(this[func])) func = "cmd_" + this[func]; // aliases.
  if (!functionp(this[func])) return this.usage;
  return this[func](args, me);
}

[$.lib.cmd.imm.room]
cmd_h = "help"
cmd_he = "help"
cmd_hel = "help"
cmd_help(args, me) {
  return this.Help(me);
}

[$.lib.cmd.imm.room]
multiUsage = "Usage: room multi <id,id,..> show|name|desc|terrain|light ...\n\n"
  "Example: room m winch1,winch2,winch3 light 7\n"
  "         room m road* name Ouroboros Road\n"
  "         room m road* light 7\n"
  "         room m road* desc s/mistaku/mistake\n"
  "         room m a*   #K(to see what will match)#n\n"
cmd_m = "multi"
cmd_mu = "multi"
cmd_mul = "multi"
cmd_mult = "multi"
cmd_multi(args, me) {
  rooms = split((args[1]||""),",",0,1);
  subCmd = join(args[2..]," ");
  if (!rooms) return this.multiUsage;
  if (!subCmd) subCmd = "test";
  saveLoc = me.location;
  dir = parent(saveLoc);
  if (!dir) return "Not in a room.";
  found=0;
  try {
    foreach(id in rooms) {
      if (id[-1] == "*") {
        // prefix wildcard for Pomke.
        pre = id[0..-2];
        foreach(name in dir) {
          if (search(name, pre) == 0 && name[0] != "@") {
            if (subCmd == "test") {
              me.Message("Your ids match: "+name);
            } else {
              me.location = dir[name];
              me.Message("In room '"+name+"':");
              err = this.Command(subCmd, me);
              if (stringp(err)) me.Message(err);
              me.Message("");
            }
            found=1;
          }
        }
      } else {
        room = this.FindPartial(dir, id);
        if (room) {
          if (subCmd == "test") {
            me.Message("Your ids match: "+id);
          } else {
            me.location = room;
            me.Message("In room '"+id+"':");
            err = this.Command(subCmd, me);
            if (stringp(err)) me.Message(err);
            me.Message("");
          }
        } else {
          me.Message("Room not found: "+id);
        }
        found=1;
      }
    }
    me.location = saveLoc;
  } catch(e) {
    me.location = saveLoc;
    me.ReportError(e);
  }
  if (!found) return "No matching rooms found.";
  return 1;
}

[$.lib.cmd.imm.room]
cmd_setid(args, me) {
  to = args[1];
  if (!to) return "Usage: room setid <new-id>   Enter 'room help' for help.";
  if (!this.ValidId(to)) return "Invalid room id: "+this.invalidIdMsg;
  room = me.location;
  dir = parent(room);
  if (!dir) return "Not in a room.";
  if (exists(dir, to)) return "Room already exists: "+to;
  move_object(room, dir, to);
  me.Message("Changed room ID to: " + to);
  return 1;
}

[$.lib.cmd.imm.room]
cmd_delete(args, me) {
  name = args[1];
  if (!name) return "Usage: room delete <name>   Enter 'room help' for help.";
  if (!this.ValidId(name)) return "Invalid room id: "+this.invalidIdMsg;
  room = me.location;
  dir = parent(room);
  if (!dir) return "Not in a room.";
  if (!dir[name]) return "Room not found: "+name;
  if (dir[name] == room) return "You are in that room. Please move to another room first.";
  if (this.AskYesNo(me, "Really #Rirreversibly DELETE#n the room '"+name+"' (y/n)?")) {
    delete dir[name];
    remove(dir, name);
    me.Message("Deleted room: " + name);
  } else {
    me.Message("Did nothing.");
  }
  return 1;
}

[$.lib.cmd.imm.room]
cmd_s = "show"
cmd_sh = "show"
cmd_sho = "show"
cmd_show(args, me) {
  room = me.location;
  if (!room) return "You are not in a room.";
  vars = new mapping;
  vars["room"] = room;
  me.MessageNoWrap(this.RenderTemplate(this.templateCache["room-info"], vars, me));
  return 1;
}

[$.lib.cmd.imm.room]
cmd_n = "name"
cmd_na = "name"
cmd_nam = "name"
cmd_ti = "name"
cmd_tit = "name"
cmd_titl = "name"
cmd_title = "name"
cmd_name(args, me) {
  room = me.location;
  if (!room) return "You are not in a room.";
  name = capitalise(join(args[1..], " "));
  while (name[-1] == ".") name = name[0..-2];
  name = trim(name);
  if (!name) return "Usage: room name <name>   Enter 'room help' for help.";
  if (name[0..1] == "S/") {
    // search and replace for Pomke.
    parts = split(name,"/");
    name = replace(room.name, parts[1], parts[2]);
  }
  room.undoName = room.name;
  room.name = name;
  me.Message("\n#YYou set the room name to:#W "+name+"#n\n");
  return 1;
}

[$.lib.cmd.imm.room]
cmd_d = "desc"
cmd_de = "desc"
cmd_des = "desc"
cmd_desc(args, me) {
  room = me.location;
  if (!room) return "You are not in a room.";
  desc = capitalise(join(args[1..], " "));
  if (desc[0..1] == "S/") {
    // search and replace for Pomke.
    parts = split(desc,"/");
    desc = replace(room.desc, parts[1], parts[2]);
  }
  if (!desc) return "Usage: room desc <text>   Enter 'room help' for help.";
  room.undoDesc = room.desc;
  room.desc = desc;
  me.Message("\n#YYou set the room description to:#W\n"+desc+"\n");
  return 1;
}

[$.lib.cmd.imm.room]
terrainDir = $.mud.class.room
cmd_t = "terrain"
cmd_ty = "terrain"
cmd_typ = "terrain"
cmd_type = "terrain"
cmd_te = "terrain"
cmd_ter = "terrain"
cmd_terr = "terrain"
cmd_terra = "terrain"
cmd_terrain(args, me) {
  room = me.location;
  if (!room) return "You are not in a room.";
  dir = this.terrainDir;
  name = args[1];
  if (!name) {
    // list terrains.
    me.Message("Usage: room terrain <name>\n\nThe room terrains are:");
    return this.cmd_types([], me);
  }
  class = this.FindPartial(dir, name);
  if (!class) return "No terrain matches: "+name+"\n\nTerrains: "+join(keys(dir),", ")+".\n";
  room.undoClass = room.class;
  room.class = class;
  me.Message("\n#YYou set the room terrain to:#W "+object_name(class)+"#n\n");
  return 1;
}

[$.lib.cmd.imm.room]
lightClass = $.lib.basic.roomclass
lightUsage = "Usage: room light <number>\n\n"
  "Available light levels:\n"
  "  0 - Pitch Black\n"
  "  1 - Extremely Dark\n"
  "  2 - Very Dark\n"
  "  3 - Dark\n"
  "  4 - Dimly Lit\n"
  "  5 - Poorly Lit\n"
  "  6 - Moderately Lit\n"
  "  7 - Well Lit\n"
  "  8 - Brightly Lit\n"
  "  9 - Blindingly Lit\n"
cmd_l = "light"
cmd_li = "light"
cmd_lig = "light"
cmd_ligh = "light"
cmd_light(args, me) {
  room = me.location;
  if (!room) return "You are not in a room.";
  light = to_int(args[1]);
  opts = this.lightClass.lightOptions;
  if (!light || light < 0 || light >= length(opts)) {
    return this.lightUsage;
  }
  room.undoLight = room.light;
  room.light = light;
  me.Message("\n#YYou set the room lighting to:#W "+light+" - "+opts[light]+"#n\n");
  return 1;
}

[$.lib.cmd.imm.room]
cmd_un = "undo"
cmd_und = "undo"
cmd_undo(args, me) {
  room = me.location;
  if (!room) return "You are not in a room.";
  switch (args[1]) {
    case "n":
    case "na":
    case "nam":
    case "name":
    case "ti":
    case "tit":
    case "titl":
    case "title":
      temp = room.name;
      room.name = room.undoName;
      room.undoName = temp;
      if (!room.name) room.name = temp;
      me.Message("\n#YYou restore the room name to:#W "+room.name+"#n\n");
      return 1;
    case "d":
    case "de":
    case "des":
    case "desc":
      temp = room.desc;
      room.desc = room.undoDesc;
      room.undoDesc = temp;
      if (!room.desc) room.desc = temp;
      me.Message("\n#YYou restore the room description to:#W "+room.desc+"#n\n");
      return 1;
    case "t":
    case "ty":
    case "typ":
    case "type":
    case "te":
    case "ter":
    case "terr":
    case "terra":
    case "terrai":
    case "terrain":
      temp = room.class;
      room.class = room.undoClass;
      room.undoClass = temp;
      if (!room.class) room.class = temp;
      me.Message("\n#YYou restore the room terrain to:#W "+object_name(room.class)+"#n\n");
      return 1;
    case "l":
    case "li":
    case "lig":
    case "ligh":
    case "light":
      temp = room.light;
      room.light = room.undoLight;
      room.undoLight = temp;
      if (!room.light) room.light = temp;
      me.Message("\n#YYou restore the room light to:#W "+room.light+"#n\n");
      return 1;
  }
  return "Usage: room undo name|desc|terrain|light";
}

[$.lib.cmd.imm.room]
roomTypes = $.mud.class.room
cmd_types(args, me) {
  vars = new mapping;
  vars["types"] = this.roomTypes;
  me.MessageNoWrap(this.RenderTemplate(this.templateCache["room-types"], vars, me));
  return 1;
}




> roomitem

1. fireplace [hidden]
2. sword-rack

> roomitem delete fireplace
are you sure you want to delete fireplace? (y/n)

> roomitem add lever hidden
added lever [hidden]

> roomitem lever desc A small lever extends from the wall, partially obscured by the sword-rack

> roomitem lever action pull echo $t pulls the small lever, ; areasignal opengate ; echo You hear gears grinding far to the north.

> roomitem lever action

1. pull:
    echo $t pulls the small lever.
    areasignal opengate
    echo You hear gears grinding far to the north.


       "Room IDs must be unique in each area. "
       "A good room ID is something like: quayside1 or oceanroad3\n"
       "\n"
       "room new <id>                Create a new room in the area.\n"
       "\n"



----------------------------------------------------------------- EXITS

pastet $.lib.cmd.imm.exit.exit-info

#YYou are in: #W{room.@id}   #K(use 'exit help' for help)#n

{each exit in exits}#Y{exit[0]}#n:
  Exit type: {exit[2].name}
    To room: {exit[1].@id}
     Custom: #W{exit[5]}{if not exit[5]}#K{exit[2].msg}{end if}#n
      Leave: #W{exit[3]}{if not exit[3]}#K{exit[2].leave}{end if}#n
     Arrive: #W{exit[4]}{if not exit[4]}#K{exit[2].arrive}{end if}#n

{end each}{if not room.exits}There are no exits from the room.
Use the 'dig' command to easily create more rooms.
{end if}{if room.exits}{if not exits}There is no exit in that direction.{end if}{end if}


pastet $.lib.cmd.imm.exit.exit-types

{each type in types}{type.col}{type.id}#n:
     Custom: {type.msg}
      Leave: {type.leave}
     Arrive: {type.arrive}

{end each}

pastet $.lib.cmd.imm.exit.exit-type-summary

{each type in types}{cols 12}{type.col}{type.id}#n{end cols}{end each}


[$.lib.cmd.imm.exit]
-> $.lib.basic.command

usage = "Unknown exit command. Enter 'exit help' for help."

help = "Edit room exits.\n"
       "\n"
       "The following commands change the room you are in:\n"
       "\n"
       "exit                      Display room exits.\n"
       "exit <dir> msg <msg>      Set a custom player message for the exit.\n"
       "exit <dir> leave <msg>    Set the message to the room when you leave.\n"
       "exit <dir> arrive <msg>   Set the message to the room when you arrive.\n"
       "\n"
       "exit <dir> to <room.id>   Link the exit to another room or area.\n"
       "exit <dir> remove         Remove the exit.\n"
       "exit <dir> type <type>    Set the exit type, or 'none'.\n"
       "exit <dir> door <type>    Set the kind of door, or 'none'.\n"
       "exit types                List exit types and messages.\n"
       "\n"
       "See also: room, extras.\n"

[$.lib.cmd.imm.exit]
Command(args, me) {
  args = split(args," ",0,1);
  dir = args[0];
  func = "qcmd_"+dir;
  if (!this[func]) {
    if (!me.location) return "You are not in a room.";
    if (dir) {
      if (!this.allowedExits[dir]) {
        dir = this.validExits[dir]; // short to long form.
        if (!dir) return "Not a valid direction.";
      }
    }
    verb = args[1];
    if (!verb) verb = "show";
    func = "cmd_" + verb;
  }
  if (stringp(this[func])) func = "cmd_" + this[func]; // aliases.
  if (!functionp(this[func])) return this.usage;
  return this[func](args, me, dir);
}

[$.lib.cmd.imm.exit]
qcmd_h = "help"
qcmd_he = "help"
qcmd_hel = "help"
qcmd_help = "help"
cmd_help(args, me) {
  return this.Help(me);
}

[$.lib.cmd.imm.exit]
cmd_s = "show"
cmd_sh = "show"
cmd_sho = "show"
cmd_show(args, me, dir) {
  room = me.location;
  vars = new mapping;
  vars["room"] = room;
  info = this.GetExit(room, dir);
  if (info) {
    vars["exits"] = [info];
  } else {
    vars["exits"] = room.exits;
  }
  me.MessageNoWrap(this.RenderTemplate(this.templateCache["exit-info"], vars, me));
  return 1;
}

[$.lib.cmd.imm.exit]
baseExit = $.mud.class.exit.none;
GetExit(room, dir, add) {
  if (!exists(room,"exits")) room.exits = [];
  foreach (entry in room.exits) {
    if (entry[0] == dir) {
      while (length(entry) < 3) append(entry, null);
      if (!entry[2]) entry[2] = this.baseExit;
      return entry;
    }
  }
  if (add) {
    entry = [dir, null, this.baseExit];
    append(room.exits, entry);
    return entry;
  }
  return null;
}

[$.lib.cmd.imm.exit]
UnlinkOtherSide(room, entry) {
  farRoom = entry[1];
  if (farRoom) {
    otherSide = this.GetExit(farRoom, this.oppositeDir[entry[0]]);
    // remove the other side if it links to this room
    if (otherSide && otherSide[1] == room) {
      remove(farRoom.exits, otherSide);
    }
  }
}

[$.lib.cmd.imm.exit]
cmd_o = "leave"
cmd_ou = "leave"
cmd_out = "leave"
cmd_l = "leave"
cmd_le = "leave"
cmd_lea = "leave"
cmd_leav = "leave"
cmd_leave(args, me, dir) {
  room = me.location;
  msg = join(args[2..]," ");
  info = this.GetExit(room, dir);
  if (!info) return "There is no exit "+dir+".";
  while (length(info) <= 3) append(info,null);
  info[3] = msg;
  me.Message("You set the 'leave' message to:\n"+msg);
  return 1;
}

[$.lib.cmd.imm.exit]
cmd_i = "arrive"
cmd_in = "arrive"
cmd_a = "arrive"
cmd_ar = "arrive"
cmd_arr = "arrive"
cmd_arri = "arrive"
cmd_arriv = "arrive"
cmd_arrive(args, me, dir) {
  room = me.location;
  msg = join(args[2..]," ");
  info = this.GetExit(room, dir);
  if (!info) return "There is no exit "+dir+".";
  while (length(info) <= 4) append(info,null);
  info[4] = msg;
  me.Message("You set the 'arrive' message to:\n"+msg);
  return 1;
}

[$.lib.cmd.imm.exit]
cmd_c = "msg"
cmd_cu = "msg"
cmd_cus = "msg"
cmd_cust = "msg"
cmd_custo = "msg"
cmd_custom = "msg"
cmd_m = "msg"
cmd_ms = "msg"
cmd_msg(args, me, dir) {
  room = me.location;
  msg = join(args[2..]," ");
  info = this.GetExit(room, dir);
  if (!info) return "There is no exit "+dir+".";
  while (length(info) <= 5) append(info,null);
  info[5] = msg;
  me.Message("You set the 'custom' message to:\n"+msg);
  return 1;
}

[$.lib.cmd.imm.exit]
cmd_to(args, me, dir) {
  room = me.location;
  id = args[2];
  if (!id) return "Usage: exit <dir> to <room.id>   Enter 'exit help' for help.";
  toRoom = this.FindRoomById(id);
  if (!toRoom) return "No matching room for: "+id;
  info = this.GetExit(room, dir, 1);
  // unlink the other side of the exit if it already goes somewhere.
  this.UnlinkOtherSide(room, info);
  // add the exit to this room.
  info[1] = toRoom;
  // add the other side of the exit to the destination room.
  otherSide = this.GetExit(toRoom, this.oppositeDir[dir], 1);
  otherSide[1] = room;
  me.Message("You link the "+dir+" exit to: "+this.RoomId(toRoom)+"\n\nPlease note that you must re-map the area after linking rooms, and non-adjacent rooms can mess up the map.");
  return 1;
}

[$.lib.cmd.imm.exit]
cmd_rem = "remove"
cmd_remo = "remove"
cmd_remov = "remove"
cmd_remove(args, me, dir) {
  room = me.location;
  info = this.GetExit(room, dir);
  if (!info) return "There is no exit "+dir+".";
  // unlink the other side of the exit if it goes somewhere.
  this.UnlinkOtherSide(room, info);
  // remove the exit from this room.
  remove(room.exits, info);
  me.Message("You remove the "+dir+" exit.\n\nPlease note that you must re-map the area to remove old exits from the map.");
  return 1;
}

[$.lib.cmd.imm.exit]
exitTypes = $.mud.class.exit
cmd_t = "type"
cmd_ty = "type"
cmd_typ = "type"
cmd_type(args, me, dir) {
  room = me.location;
  type = args[2];
  class = this.exitTypes[type];
  if (!class) return "No matching exit type: "+type+"\n\nType can be: "+join(keys(this.exitTypes),", ")+".";
  info = this.GetExit(room, dir);
  if (!info) return "There is no exit "+dir+".";
  info[2] = class;
  me.Message("You set the exit type to:\n");
  // show the exit messages.
  vars = new mapping;
  vars["types"] = [class];
  me.MessageNoWrap(this.RenderTemplate(this.templateCache["exit-types"], vars, me));
  return 1;
}

[$.lib.cmd.imm.exit]
qcmd_types = "types"
cmd_types(args, me) {
  // filter types matching argument.
  types = this.exitTypes;
  filter = args[1];
  if (filter) {
    if (filter == "all") filter = "";
    res = [];
    foreach (id in types) {
      if (search(id, filter) >= 0) append(res, types[id]);
    }
    // show the exit messages.
    vars = new mapping;
    vars["types"] = res;
    me.MessageNoWrap(this.RenderTemplate(this.templateCache["exit-types"], vars, me));
  } else {
    vars = new mapping;
    vars["types"] = types;
    me.MessageNoWrap(this.RenderTemplate(this.templateCache["exit-type-summary"], vars, me));
  }
  return 1;
}

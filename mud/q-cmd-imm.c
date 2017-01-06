[$.lib.cmd.imm.clone]
-> $.lib.basic.command

homeDir = $.home
itemDir = $.mud.item

Command(args, me) {
    args = trim(args);
    if (!args) return "Usage: clone <id>  e.g. imm.diary or @mario.frog";
    if (args[0] == "@") {
        dir = this.homeDir;
        id = args[1..];
    } else {
        dir = this.itemDir;
        id = args;
    }
    proto = this.FindPartial(dir, id);
    if (!proto) return "No item with this ID: "+args;
    inst = new object(proto);
    inst.Move(me, "", me.TheName()+" clones "+inst.AName()+".");
    me.Message("You clone "+inst.AName()+".");
    return 1;
}

[$.lib.cmd.imm.spawn]
-> $.lib.basic.command

homeDir = $.home
mobDir = $.mud.mob

Command(args, me) {
    args = trim(args);
    if (!args) return "Usage: spawn <id>  e.g. gaspode";
    if (!me.location) return "You are in limbo.";
    if (args[0] == "@") {
        dir = this.homeDir;
        id = args[1..];
    } else {
        dir = this.mobDir;
        id = args;
    }
    proto = this.FindPartial(dir, id);
    if (!proto) return "No mob with this ID: "+args;
    inst = proto.Clone(me);
    inst.Move(me.location, "", me.TheName()+" spawns "+inst.AName()+".");
    me.Message("You spawn "+inst.AName()+".");
    return 1;
}

[$.lib.cmd.imm.list]
-> $.lib.basic.command

Command(args, me) {
    args = trim(args);
    if (!args) return "Usage: list <item>";
    item = this.ParseMatch(args, me);
    if (!item) return "Could not find " + args + ".";
    s = "\n"
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


[$.lib.cmd.admin.newarea]
-> $.lib.basic.command
roomDir = $.area
baseArea = $.lib.basic.area
usage = "Create a new area.\nUsage: newarea <domain>.<area>"

Command(args, me)  {
    words = split(trim(args), " ");
    if (length(words) != 1) return this.usage;
    path = split(words[0], ".");
    if (length(path) != 2) return this.usage;
    domain = path[0];
    newId = path[1];
    if (!this.ValidId(newId)) return "Invalid area ID: "+this.invalidIdMsg;
    if (!exists(this.roomDir, domain)) return "No such domain.";
    dir = this.roomDir[domain];
    if (exists(dir, newId)) return "Area already exists: "+newId;
    area = new object(newId, dir);
    meta = new object("@meta", area);
    add_inherit(meta, this.baseArea);
    meta.name = capitalise(newId);
    me.Message("Created area "+newId+" in domain "+domain+".");
    return 1;
}


[$.lib.cmd.imm.newroom]
-> $.lib.basic.command
roomDir = $.area
baseRoom = $.lib.basic.room
usage = "Create a new room in an area.\nUsage: newroom <domain>.<area> <roomid>"

Command(args, me)  {
    words = split(trim(args), " ");
    if (length(words) != 2) return this.usage;
    areaId = words[0];
    newId = words[1];
    if (!this.ValidId(newId)) return "Invalid room ID: "+this.invalidIdMsg;
    dir = find_object(areaId, this.roomDir);
    if (!dir) return "No such area: "+areaId;
    meta = dir["@meta"];
    if (!meta) return "Area has no meta: "+areaId;
    if (exists(dir, newId)) return "Room already exists: "+newId;
    room = new object(newId, dir);
    add_inherit(room, this.baseRoom);
    room.name = capitalise(newId);
    room.area = meta; // link room to area info.
    if (!meta.origin) meta.origin = room; // set map origin.
    me.Message("Created room "+newId+" in area "+areaId+".");
    return me.DoCommand("goto "+object_path(room));
}



[$.lib.cmd.imm.ccat]
-> $.lib.basic.command
Command(args, me) {
    args = trim(replace(args,"^",""));
    if (search(args,"$.home.") == 0) {
        src = this.FindPathValue(args,$);
        me.MessageNoWrap(src);
    }
    return 1;
}

[$.lib.cmd.imm.cpat]
-> $.lib.basic.command
Command(args, me) {
    args = trim(replace(args,"^",""));
    args = split(args, " ");
    if (search(args[0],"$.home.") == 0 && args[1]) {
        a = split(this.FindPathValue(args[0],$), "\n");
        b = ["   ", join(args[1..], " "), "    "];
        len = this.LengthCC(a[0]);
        s=""; for (i=0;i<3;i+=1) { s += this.MergePatterns(a[i], b[i], len, 0, 0)+"\n"; }
        me.Message(s);
    }
    return 1;
}



[$.lib.cmd.imm.emotes]
-> $.lib.basic.command

funcDir = $.lib.funcs
verbDir = $.verbs

Command(args, me) {
  verbDir = this.verbDir;
  if (!args) {
    s = "#m |\n"
        "#m-+----------------\n"
        "#m |#C Quintiqua Emotes:\n"
        "#m-+------------------------------------\n"
        "#m |#n Use 'addemote' to add or modify emote commands.\n"
        "#m |\n";
    col = 0;
    foreach(name in verbDir) {
      verb = verbDir[name];
      if (verb.func == "emote") {
        s += "#m | #Y" + this.PadString(name[0..15], 16);
        col += 1;
        if (col >= 4) {
          s += "#n\n";
          col = 0;
        }
      }
    }
    s += "#n\n";
    me.Message(s);
    return 1;
  }
  if (!exists(verbDir, args)) {
    return "There is no emote named: "+args;
  }
  cmd = verbDir[args];
  s = "";
  if (arrayp(cmd.self)) {
    me.Message(
      "\nWithout a target,\n"
      "You see:      #GYou " + cmd.self[0] + "#n\n"
      "Others see:   #G"+me.Name()+" " + cmd.self[1] + "\n");
  }
  if (arrayp(cmd.other)) {
    me.Message(
      "\nWith a target,\n"
      "You see:      #GYou " + replace(cmd.other[0],"$t","Bob") + "#n\n"
      "Others see:   #G"+me.Name()+" " + replace(cmd.other[1],"$t","Bob") + "#n\n"
      "Target sees:  #G"+me.Name()+" " + (cmd.other[2]||"") + "\n");
  }
  return 1;
}


[$.lib.cmd.imm.addemote]
-> $.lib.basic.command

funcDir = $.lib.funcs
verbDir = $.verbs
help = "Add an emote command.\n\n"
       "Use 'addemote <verb> <message>' to add an emote.\n\n"
       "Example: addemote dance dance/s around the room.\n"
       "         addemote dance dance/s around the room with $t.\n\n"
       "You can add both of these at once, with and without $t.\n"

Command(args, me) {
  if (!args) {
    return this.Help(me);
  }
  verbDir = this.verbDir;
  funcDir = this.funcDir;
  words = split(args, " ", 0, 1);
  name = words[0];
  msg = join(words[1..], " ");
  if (skip(name, 0, "abcdefghijklmnopqrstuvwxyz") >= 0) {
    return "Verbs must use only lower-case letters. See 'addemote' for help.";
  }
  if (!msg) {
    return "Must supply a message for the emote. See 'addemote' for help.";
  }
  if (exists(verbDir, name) && verbDir[name].func != "emote") {
    return "Verb already exists and is not an emote.";
  }
  if (exists(verbDir, name)) {
    cmd = verbDir[name];
  } else {
    cmd = new object(name, verbDir);
    cmd.name = name;
    cmd.func = "emote";
    cmd.need = 1;
  }
  if (search(msg, "$t") < 0) {
    cmd.self = [
      replace(replace(msg, "/s", ""), "/es", ""),
      replace(msg, "/", "")
    ];
    me.Message(
      "You see:      #GYou " + cmd.self[0] + "#n\n"
      "Others see:   #G"+me.Name()+" " + cmd.self[1] + "\n");
  } else {
    other = replace(msg, "/", "");
    cmd.other = [
      replace(replace(msg, "/s", ""), "/es", ""),
      other,
      replace(replace(other, "$t", "you"), "you's", "your")
    ];
    me.Message(
      "You see:      #GYou " + replace(cmd.other[0],"$t","Bob") + "#n\n"
      "Others see:   #G"+me.Name()+" " + replace(cmd.other[1],"$t","Bob") + "#n\n"
      "Target sees:  #G"+me.Name()+" " + (cmd.other[2]||"") + "\n");
  }
  return 1;
}





[$.lib.cmd.imm.comments]
-> $.lib.basic.command

notesGuide = "Please include your name in [brackets] before your comment. It is considered rude to remove others' comments without their permission. Please be aware that only one person can safely modify the room comments at a time."

Command( args, me )
{
    // Edit comments for a room

    if( !me.location ) return "You are in limbo.";
    oRoom = me.location;

    comments = oRoom.comments;
    if( !stringp(comments) ) comments = "";

    sTitle = "Quintiqua Room Notes: " + object_path(oRoom) + "\n\n" + this.notesGuide;
    sComments = this.SimpleEdit( me, sTitle, oRoom.comments );
    if (stringp(result)) {
        oRoom.comments = result;
    }

    return 1;
}


[$.lib.cmd.imm.dest]
-> $.lib.basic.command

Command( args, me )
{
    if( !(here = me.location) ) return "You are in limbo.";

    if( length(args) )
    {
        // Find the target
        target = find_object(args, $);
        if (!target) {
            target = this.ParseMatch(args, me);
            if( !target ) return "There is no " + args + " here.";
        }

        // Check that the object is a clone
        if( !clonep(target) ) return "Not a clone: " + object_path(target);

        // Move the clone into limbo
        sName = target.TheName();
        target.Move( null, me.TheName() + " disintegrates " + sName + ".", "" );

        // Delete the object
        sName += " (" + object_path(target) + ")\n";
        delete target;
        me.RawMessage( "Destructed: " + sName );
        return 1;
    } else {
        s = "Usage: dest <id>\n\n";
        remove( here.contents, null );
        foreach( oThing in here.contents ) {
            if (clonep(oThing)) {
                s += to_string(oThing) + " [" + oThing.id + "] " + oThing.name + "\n";
            }
        }
        me.RawMessage(s);
        return 1;
    }
}

[$.lib.cmd.imm.dig]
-> $.lib.basic.command
roomDir = $.area
baseRoom = $.lib.basic.room
exitClasses = $.mud.class.exit

help = "Dig an exit to an adjacent room or a new room.\n"
       "\n"
       "Usage: dig <direction> <kind> [.]\n"
       "       dig <direction> <kind> <new-id> [.]\n"
       "       dig <direction> <kind> <domain>.<area>.<room-id> [.]\n"
       "\n"
       "If there is already a room on the map in the direction you dig, "
       "the dig command will join the exit to that room.\n"
       "\n"
       "  Example: dig n avenue\n"
       "\n"
       "Otherwise, you must supply a new room-id for the room to create.\n"
       "\n"
       "  Example: dig n door toyshop1\n"
       "  Example: dig n door toyshop*   (to use the next number)\n"
       "\n"
       "Each room in the area must have a unique room-id, which must be a single "
       "word made from lower-case letters, numbers and dashes.\n"
       "\n"
       "You can link to another area using a fully-qualified room id:\n"
       "\n"
       "  Example: dig n door faerryn.radakara.town17\n"
       "\n"
       "If you add an extra . (dot) after the command, "
       "you will stay in the same room, otherwise you will follow the exit.\n"
       "\n"
       "Direction can be: n, e, s, w, u, d, nw, ne, sw, se.\n"
       "\n"

Help(me) {
    return this.help +
        "Kind can be: " + join(keys(this.exitClasses), ", ") + ".";
}

[$.lib.cmd.imm.dig]
dirOfsX = { "n":0, "s":0, "e":1, "w":-1, "u":0, "d":0, "nw":-1, "ne":1, "sw":-1, "se":1 }
dirOfsY = { "n":-1, "s":1, "e":0, "w":0, "u":0, "d":0, "nw":-1, "ne":-1, "sw":1, "se":1 }
dirOfsZ = { "n":0, "s":0, "e":0, "w":0, "u":1, "d":-1, "nw":0, "ne":0, "sw":0, "se":0 }

[$.lib.cmd.imm.dig]
AddExit(fromRoom, dir, toRoom, exitClass) {
    found = 0;
    foreach (entry in fromRoom.exits) {
        if (entry[0] == dir) {
            entry[1] = toRoom;
            entry[2] = exitClass;
            found = 1;
            break;
        }
    }
    if (!found) {
        // Add an exit from this room to the destination
        fromRoom.exits += [ [ dir, toRoom, exitClass ] ];
    }
}

[$.lib.cmd.imm.dig]
Command(args, me)
{
    fromRoom = me.location;
    if (!fromRoom) return "You are in limbo.";
    area = fromRoom.area;
    if (!area) return "This room is not part of an area.";

    words = split(args, " ", 0, 1); // trimmed split.
    shortDir = lower_case(words[0]);
    sClass = words[1];
    newID = words[2]; // optional.

    if (length(words) < 2) return this.Help(me);

    // Check the exit direction
    sDir = this.validExits[shortDir];
    if (!sDir) return "Not a valid direction: " + shortDir;
    sOpposite = this.oppositeDir[sDir];
    if (!sOpposite) return "No opposite direction for: " + sDir;

    // Validate the exit class
    if (!exists(this.exitClasses, sClass)) return "Not a valid kind of exit: " + sClass;
    exitClass = this.exitClasses[sClass];

    // Check for an adjacent room on the map.
    xpos = fromRoom.xpos + this.dirOfsX[shortDir];
    ypos = fromRoom.ypos + this.dirOfsY[shortDir];
    zpos = fromRoom.zpos + this.dirOfsZ[shortDir];
    adjRoom = area.RoomAt(xpos, ypos, zpos);
    if (adjRoom) {
        // Link to existing room, unless newID was provided and does not match.
        if (newID && newID != "." && newID != object_name(adjRoom)) {
            return "\n#YA room already exists to the "+ sDir +": "+object_name(adjRoom)+"#n\nYou can leave off the room-id to link to the adjacent room.\n";
        }

        // Link the exit to the other room and back.
        this.AddExit(fromRoom, sDir, adjRoom, exitClass);
        this.AddExit(adjRoom, sOpposite, fromRoom, exitClass);

        me.MessageWrap("\nAdded exit " + sDir + " to room " + object_name(adjRoom) + "\n\n");

        // Go to the room.
        if (newID == ".") return 1;
        newID = object_name(adjRoom);
    } else {
        if (!newID) return "#YThere is no adjacent room to the "+sDir+".#n\nYou must provide a new room-id when digging a new room.\n";
        if (search(newID, ".") > 0) {
            // Link to a room in another area.
            farRoom = find_object(newID, this.roomDir);
            if (!farRoom) return "Cross-area room not found: "+newID;
            if (inherit_list(farRoom)[0] != this.baseRoom) return "Not a room: "+newID;

            // Link the exit to the far room and back.
            this.AddExit(fromRoom, sDir, farRoom, exitClass);
            this.AddExit(farRoom, sOpposite, fromRoom, exitClass);

            me.MessageWrap("\n#YLinked the "+sDir+" exit to '" + newID + "'.\n\n");
            if (farRoom.area == area) {
                me.MessageWrap("#RSince both rooms are in the same area, you will need to 'remap' the area.\n\n");
            }
            if (words[3] == ".") return 1;

        } else {
            // Dig a new room and link to it.

            if (newID[-1] == "*" && length(newID) >= 2) {
                // Find the next consecutive room number.
                baseId = newID[0..-2];
                foreach(id in
            }
            if (!this.ValidId(newID)) return "Bad room-id: "+this.invalidIdMsg; // global.

            // Make sure the room-id is not already taken.
            if( find_object( newID, parent(fromRoom) ) )
                return "#YRoom already exists, and is not adjacent: " + newID;

            // Check if the exit already exists in this direction.
            foreach( arExit in fromRoom.exits ) {
                if( arExit[0] == sDir ) return "#YExit already exists: " + sDir;
            }

            // Create a new room
            newRoom = new object( newID, parent(fromRoom) );
            add_inherit(newRoom, this.baseRoom);
            newRoom.area = fromRoom.area;
            newRoom.class = fromRoom.class;
            newRoom.light = fromRoom.light;
            newRoom.name = fromRoom.name;
            newRoom.xpos = xpos;
            newRoom.ypos = ypos;
            newRoom.zpos = zpos;
            area.SetRoomAt(xpos, ypos, zpos, newRoom);

            // Link the exit to the new room and back.
            this.AddExit(fromRoom, sDir, newRoom, exitClass);
            this.AddExit(newRoom, sOpposite, fromRoom, exitClass);

            me.MessageWrap("\n#YCreated room '" + newID + "' to the " + sDir + ".\n\n");
            if (words[3] == ".") return 1;
        }
    }

    return me.DoCommand("goto "+newID);
}


[$.lib.cmd.imm.port]
-> $.lib.basic.command

help =  "Teleport to a player's location.\n"
        "\n"
        "Usage: port <name>\n"
        "\n"
        "You can enter a partial name to teleport to an imm, "
        "however you must enter the full name of a normal player.\n"

[$.lib.cmd.imm.port]
Command(args, me) {
    name = trim(args);
    if (!name) return this.help;
    player = this.loginDaemon.FindUserExact(name);
    if (!player) player = this.loginDaemon.FindImm(name);
    if (!player) return "Cannot find player: " + name;
    room = player.location;
    if (!room) return "Player is in limbo: "+ player.name;
    if (me.location == room) return "You are already there.";
    if (me.location) me.prevLocation = me.location;
    return me.Move(room, me.Name()+" "+me.portOutMsg, me.Name()+" "+me.portInMsg);
}


[$.lib.cmd.imm.return]
-> $.lib.basic.command

help =  "Return to your previous location, before the last teleport.\n"

Command(args, me) {
    room = me.prevLocation;
    if (!room) return "You do not have a previous location.";
    if (me.location == room) return "You are already there.";
    if (me.location) me.prevLocation = me.location;
    return me.Move(room, me.Name()+" "+me.portOutMsg, me.Name()+" "+me.portInMsg);
}


[$.lib.cmd.imm.goto]
-> $.lib.basic.command

help =  "Go to a room in this area or in another area.\n"
        "\n"
        "   goto <room>\n"
        "\n"
        "The goto command will find the first room-id that starts with <room> in "
        "the area you are in. See 'rooms' for a list of rooms in the area. You "
        "can also use any full room-id that appears when you 'look'.\n"
        "\n"
        "   goto <area> <room>\n"
        "\n"
        "When you include an <area> it will search for a matching area within "
        "the domain you are in, then in all other domains.\n"
        "\n"
        "   goto <area> .\n"
        "\n"
        "You can use . (dot) instead of a room-id to go to the center of an area. "
        "The center is the room at 0,0,0. You can set the center room with the "
        "'area' command.\n"
        "\n"
        "   goto <domain> <area> <room>\n"
        "   goto <domain> <area> .\n"
        "   goto d <domain>\n"
        "\n"
        "Include a domain name if the area name is ambiguous. Use 'goto d' "
        "to go to the common room of a domain, or show a list of domains.\n"

areasDir = $.area
baseRoom = $.lib.basic.room

GoTo(me, room) {
    if (me.location == room) return "You are already there.";
    if (me.location) me.prevLocation = me.location;
    return me.Move(room, me.Name()+" "+me.portOutMsg, me.Name()+" "+me.portInMsg);
}

[$.lib.cmd.imm.goto]
Command(args, me) {
    args = trim(args);
    if (!args) return this.help;
    words = split(args, " ");
    if (length(words) > 3) return this.help;
    // special case: goto d <domain>
    if (words[0] == "d") {
        if (length(words) > 1) {
            // go to domain common room.
            domain = this.FindPartial(this.areasDir, words[1]);
            if (!domain) return "No matching domain: "+words[1];
            room = this._get_path(domain,["@meta","common"]);
            if (!room) return "The domain "+object_name(domain)+" has no common room. Please use the 'area' command to set one.";
            return this.GoTo(me, room);
        } else {
            // list domains.
            return "Available domains: "+join(keys(this.areasDir),", ");
        }
    }
    // special case: full room-id.
    if (length(words) == 1 && search(words[0], ".") > 0) {
        room = find_object(words[0], this.areasDir);
        if (!room) return "Room not found: " + words[0];
        return this.GoTo(me, room);
    }
    // all other cases.
    domain = null;
    area = null;
    room = null;
    if (length(words) == 3) {
        // goto <domain> <area> <room>
        domain = this.FindPartial(this.areasDir, words[0]);
        if (!domain) return "No matching domain: "+words[0];
        area = this.FindPartial(domain, words[1]);
        if (!area) return "No matching area in domain "+object_name(domain)+": "+words[1];
    } else if (length(words) == 2) {
        // goto <area> <room>
        if (me.location) {
            domain = parent(parent(me.location));
            area = this.FindPartial(domain, words[0]);
        }
        if (!area) {
            // look in all domains for a match.
            foreach (name in this.areasDir) {
                domain = this.areasDir[name];
                area = this.FindPartial(domain, words[0]);
                if (area) break;
            }
        }
        if (!area) return "No matching area in any domain: "+words[0];
    } else {
        // goto <room>
        if (me.location) {
            area = parent(me.location);
            domain = parent(area);
        } else {
            return "This room is not part of an area.";
        }
    }
    // find the room in the area we have found.
    areaPath = object_name(domain)+"."+object_name(area);
    if (words[-1] == ".") {
        room = this._get_path(area,["@meta","origin"]);
        if (!room) return "The area "+areaPath+" has no origin room. Please use the 'area' command to set one.";
    } else {
        // find room in areasDir.
        room = this.FindPartial(area, words[-1]);
        if (!room) return "No matching room in "+areaPath+": "+words[-1];
    }
    return this.GoTo(me, room);
}


[$.lib.cmd.imm.home]
-> $.lib.basic.command
home = $.home
userList = $.save.user

Command( args, me )
{
    if( length(args) )
    {
        oUser = this.loginDaemon.GetUser( args );
        if( !oUser ) return "No such user '" + args + "'.";
        args = oUser.username;
    }
    else args = me.username;

    if (!objectp(this.home[args])) {
        return "User '" + args + "' has no home directory.";
    }

    dir = this.home[args];

    if (!objectp(dir.workroom)) {
        return "User '" + args + "' has no home room.";
    }

    room = dir.workroom;
    if (me.location == room) return "You are already there.";

    if (me.location) me.prevLocation = me.location;
    return me.Move(room, me.Name()+" "+me.portOutMsg, me.Name()+" "+me.portInMsg);
}


[$.lib.cmd.imm.map]
-> $.lib.basic.command

Command(args, me) {
    here = me.location;
    if (!here) return "You are in limbo.";
    area = here.area;
    if (!area) return "This room is not assigned to an area.";
    if (!area.map) return "This area has no map. Please use 'remap' to create the map.";
    if (args == "old") {
        map = area.RenderMap2(here);
    } else {
        map = area.GenerateImmMap(me, to_int(args));
    }
    me.MessageNoWrap(map);
    return 1;
}



[$.lib.cmd.imm.page]
-> $.lib.basic.command

Command( args, me )
{
    if( args )
    {
        // Split into name and message
        nPos = search( args, " " );
        if( nPos > -1 )
        {
            // Split at the first space
            sName = args[0..nPos - 1];
            args = args[nPos + 1..];

            // Find the user
            oUser = this.loginDaemon.GetUser( sName );
            if( !oUser ) return "No such user: " + sName;

            args = replace( args, "$", "$$" );
            if( !me.creator ) args = replace( args, "#", "##" );

            // Send the page to the user
            return oUser.ReceivePage( me, args );
        }

        return "Usage: page <user> <stuff>";
    }

    me.DisplayPageHistory();
    return 1;
}


[$.lib.cmd.imm.remap]
-> $.lib.basic.command

Command(args, me)
{
    if (!me.location) return "You are in limbo.";
    area = me.location.area;
    if (!area) return "This room is not part of an area.";
    if (!area.origin) return "This area has no origin room. Please use the 'area' command to set one.";
    me.Message( "Remapping " + area.name + "..." );
    if (area.UpdateAreaMap(me)) return "Done.";
    return "Failed to build area map.";
}


[$.lib.cmd.imm.trans]
-> $.lib.basic.command

Command( args, me )
{
    if( length(args) )
    {
        here = me.location;

        // Find the target user by name
        target = this.loginDaemon.FindUser( args );
        if( target )
        {
            if( !target.location ) return "User has no location.";
            if( target.location == here ) return target.TheName() + " is already here.";
        }
        else return "User is not logged on: " + args;

        // Notify the user being moved
        target.Message( me.TheName() + " teleports you to " + replace( object_path( here ), "$", "$$" ) );

        // Move the user
        return target.Move( here, target.TheName() + " teleports out.",
          target.AName() + " teleports in." );
    }

    return "Usage: trans <user>";
}


[$.lib.cmd.imm.where]
-> $.lib.basic.command

Command( args, me ) {
    s = "";
    foreach( user in this.loginDaemon.Users() ) {
        idle = time() - user.lastInput;
        days = to_int(idle / 86400);
        daySub = days * 86400;
        idle = to_int(idle - daySub);
        hrs = to_int(idle / 3600);
        hrsSub = hrs * 3600;
        idle = to_int(idle - hrsSub);
        mins = to_int(idle / 60);
        minSub = mins * 60;
        idle = to_int(idle - minSub);
        s += "#C" + user.name + "#n[#Y";
        s += to_string(user.location) + "#n] ("+days+":"+hrs+":"+mins+":"+idle+" idle) \n";
    }
    me.Message(s);
}

{each user in $.loginDaemon.onLineList}#C{user.name}#n {user.location.name}#n [#Y{user.location.@id}#n]
{end each}


[$.lib.cmd.imm.imm]
-> $.lib.basic.command
chatsCmd = $.lib.cmd.user.immhist
history = []

Command( args, me )
{
    if (!me.creator) return "You are not an imm.";
    if (args) {
        if (args[0..0] == "@" ) {
            // emote
            result = "#m[imm] " + me.name + " " + args[1..];
            arAppend = [ [ me, args[1..], time(), 1 ] ];
        } else {
            // say
            result = "#m[imm] " + me.name + ":#n " + args;
            arAppend = [ [ me, args, time(), 0 ] ];
        }

        // Add this to the chat history
        if (length(this.history) < 1000 ) this.history += arAppend;
        else this.history = this.history[-999..] + arAppend;

        // Broadcast the chat to all imms.
        foreach (user in this.loginDaemon.Users()) {
          if (user.creator) {
            user.Message(result);
          }
        }
        return 1;
    } else {
        // Display chat history.
      return this.chatsCmd.Command("30 all", me);
    }
}


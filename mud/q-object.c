[$.lib.basic.object]
-> $.lib.global

perms = 3
permNames = ['user','mob','imm','admin']
commands = $.lib.cmd.user
Close = null
Open = null
Read = null
actions = []
adjectives = ["formless", "blob"]
ambient = 0
container = 0
contents = []
damage = 0
desc = "This is the stuff from which all is made."
desc2 = null
events = {}
held = 0
id = "thing,"
idadj = ""
idplural = "things,"
isWorn = 0
keyname = null
living = 0
location = null
mass = 100
name = "thing"
npc = 0
noclone = 1
plural = "things"
radiant = 0
unique = 0
value = 0
visible = 1
wander = 0
worn = 0

[$.lib.basic.object]
commandPrefixes = {
  "'": "say",
  ":": "emote",
  "@": "act",
  "^": "act"
}

Message()
{
}

AName( oAgent, bColour )
{
    sName = this.Name();
    sFirst = sName[0..0];

    if( bColour ) sName = "#Y" + sName + "#n";
    if( this.unique ) return sName;
    if( upper_case(sFirst) == sFirst ) return sName;

    if( search( "aeiouAEIOU", sFirst ) >= 0 ) return "an " + sName;

    return "a " + sName;
}

[$.lib.basic.object]
DoCommand(cmd, vars, oldCmds) {
    pos = skip(cmd); // skip spaces
    space = search(cmd, " ", pos);
    if (space > pos) {
        verb = cmd[pos..space-1];
        args = cmd[space+1..];
    } else {
        verb = trim(cmd);
        args = "";
    }
    act = $.verbs[verb];
    if (act) {
        func = act.func;
        if (functionp(func)) {
            // native code commands.
            res = func(args, vars);
        } else {
            cmds = act.cmds;
            if (arrayp(cmds)) {
                // execute a command list.
                local = new mapping;
                foreach(k in vars) local[k] = vars[k];
                local["@verb"] = act;
                local["args"] = args;
                local["self"] = this;
                foreach (line in cmds) {
                    res = this.DoCommand(line, local);
                    if (res) break;
                }
            } else if (stringp(cmds)) {
                // an alias for another command.
                res = this.DoCommand(cmds, vars);
            } else {
                if (stringp(func)) {
                    // legacy func commands.
                    fn = $.funcs[func];
                    if (functionp(fn)) {
                        res = fn(act, args, this, vars);
                    } else {
                        res = "Missing function: "+func;
                    }
                } else {
                    res = "Bad verb: "+verb;
                }
            }
        }
    } else if (oldCmds) {
        // check for an old command.
        func = this.commands[verb].Command;
        if (functionp(func)) {
            res = func(args, this, vars);
        } else {
            res = "What?";
        }
    } else {
        res = "What?";
    }
    return res;
}

[$.lib.basic.object]
Event(name, vars) {
    cmds = this.events[name];
    if (arrayp(cmds)) {
        // command list.
        foreach (line in cmds) {
            result = this.DoCommand(line, vars);
            if (stringp(result)) {
              return "In ["+line+"] : "+result;
            }
        }
        return 1;
    }
}

Clone( oAgent )
{
    if( this.noclone ) return null;
    oClone = new object( this );
    if( oClone.living ) this.npcDaemon.RegisterNpc( oClone );
    return oClone;
}

ContentMass()
{
    return 0;
}

Describe()
{
    // Use desc2 to simplify base and derived room descriptions
    if( this.desc2 ) return this.desc + " " + this.desc2;
    return this.desc;
}

Emote( sText )
{
    sMessage = "#g" + capitalise(this.TheName()) + " " + sText;
    if (this.location) this.location.Broadcast( sMessage, this );
}

Light( nAmbient )
{
    return this.radiant;
}

[$.lib.basic.object]
Move( oLocation, sLeave, sArrive, oAgent, custom )
{
    // This might go into the living object..
    if( oLocation && !functionp(oLocation.AddObject) ) return "Not a room.";

    // Move out of the old location (if a valid room)
    oOldLoc = this.location;
    if( oOldLoc && functionp(oOldLoc.RemoveObject) )
    {
        oOldLoc.RemoveObject( this );
        if (sLeave) oOldLoc.Broadcast( "#W" + capitalise(sLeave), oAgent );
    }

    if (oAgent && oAgent.creator && sLeave) {
        this.Message("#K[Leave: "+capitalise(sLeave)+"]#n\n\n");
    }

    // Store the new location reference
    this.location = oLocation;

    // Notify player if there is a custom message.
    if (stringp(custom)) this.Message("\n#W"+custom+"#n\n\n");

    // Move into the new location
    if( oLocation )
    {
        if (sArrive) oLocation.Broadcast( "#W" + capitalise(sArrive), oAgent );
        oLocation.AddObject( this );
    }

    // If a user, look around the new location
    if( oLocation && functionp(this.Look) ) this.Look();

    if (oAgent && oAgent.creator && sArrive) {
        this.Message("\n#K[Arrive: "+capitalise(sArrive)+"]#n");
    }

    return 1;
}

Name()
{
    return this.name;
}

Perform( sText )
{
    sMessage = capitalise(this.TheName()) + " " + sText;
    if (this.location) this.location.Broadcast( sMessage, this );
}

Plural()
{
    if( this.plural ) return this.plural;
    sName = this.Name();
    if( sName[-1..-1] == "s" ) return sName + "es";
    return sName + "s";
}

[$.lib.basic.object]
RandomAction() {
    // Wander a third of the time.
    if (this.wander) {
        if( !random(3) ) return this.MoveRandom();
    }
    // Select an action at random.
    actionList = this.actions;
    nLen = sizeof(actionList);
    if (nLen) {
        cmd = actionList[random(nLen)];
        if (stringp(cmd)) {
            alias = this.commandPrefixes[cmd[0]];
            if (alias) {
                cmd = alias + " " + cmd[1..];
            } else if (this.isRoom) {
                cmd = "act " + cmd; // HACK for old room actions.
            }
            result = this.DoCommand(cmd, new mapping);
            if (stringp(result)) {
              result = "In "+this.Name()+" ["+cmd+"] : "+result;
              result = replace(result,"$","$$");
              // TODO: send to imms in the room.
              room = this.location;
              if (room) room.Broadcast(result, this);
            }
        }
    }
}

[$.lib.basic.object]
Speak(chan, msg, action) {
    room = this.location;
    if (room) {
      if (room.location) room = room.location; // in player.
      room.Broadcast(action + msg, this);
    }
}

Say( sText )
{
    color + "$^agent " + chan + ":s#n "
    sMessage = "#c" + capitalise(this.TheName()) + " says:#n " + sText;
    if (this.location) this.location.Broadcast( sMessage, this );
}

Short( oAgent )
{
    return this.Name( oAgent, 1 );
}

TestKey( sKey, oLock, oAgent )
{
    if( !this.keyname || this.keyname != sKey )
        return this.TheName() + " does not fit in the lock.";

    return 1;
}

TheName( oAgent, bColour )
{
    sName = this.Name();
    sFirst = sName[0..0];

    if( bColour ) sName = "#Y" + sName + "#n";
    if( this.unique ) return sName;
    if( upper_case(sFirst) == sFirst ) return sName;

    if( this.location && this.location.TestPresent( this.name, this ) )
    {
        sName = this.Plural();
        if( bColour ) sName = "#Y" + sName + "#n";
        return "one of the " + sName;
    }

    return "the " + sName;
}

TotalMass()
{
    return this.mass;
}

UseKey( sKey, oLock, oAgent )
{
}

Value()
{
    return this.value;
}

Visible( oAgent )
{
    if( !this.visible ) return 0;
    return 1;
}

RenderMessage(msg, vars)
{
    ob = "{";
    ofs = 0;
    open = search(msg, ob, ofs);
    if (open < 0) return msg; // no { in message
    cb = "}"
    result = "";
    repeat {
        // keep the part before the open bracket
        if (open > pos) result += msg[pos..open-1];

        close = search(msg, cb, open);
        if (close <= pos) return result; // missing }

        // switch on the first character
        switch (msg[open+1]) {
        case '@':
          // repeat over the contents of expression
          break;
        case '?':
          // include if expression is true
          break;
        case '!':
          // include if expression is false
          break;
        }
    }
}

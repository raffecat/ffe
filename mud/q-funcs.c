[$.verbs.say]
cmds = ["echo #cYou say:#n $args", "msg-room #c$n says:#n $args"]
func = "cmd"
name = "say"
need = 1

[$.lib.funcs]
cmd (act, args, me, vars) {
  // invoke a command-list.
  // this should not be a command, but rather part of DoCommand.
  if (!arrayp(act.cmds)) return "No command list.";
  local = new mapping;
  foreach(key in vars) local[key] = vars[key];
  local["me"] = me;
  local["room"] = me.location;
  local["args"] = args;
  local["@verb"] = act;
  foreach (line in act.cmds) {
    result = me.DoCommand(line, local);
    if (result != 1) {
      if (!stringp(result)) result = "Failed.";
      return "In ["+line+"] : "+result;
    }
  }
  return 1;
}

[$.lib.funcs]
msgto (act, args, me, vars) {
  // write a message with placeholders to a target.
  ofs = search(args," ");
  if (ofs < 0) return "Usage: msg-to <target> <message>";
  to = args[0..ofs-1];
  msg = args[ofs+1..];
  // resolve target.
  if (to[0] == "$") to = vars[to[1..]];
  if (stringp(to)) to = vars[to];
  if (!objectp(to)) return "No target.";
  if (!functionp(to.Message)) return "Bad target.";
  // render message.
  msg = $.ExpandMessage(msg, vars);
  to.Message(msg);
  return 1;
}

[$.lib.funcs]
msgroom (act, args, me, vars) {
  // write a message with placeholders to the room of self.
  // neither self nor target will see the message.
  if (me.location) {
    target = vars["target"];
    foreach (thing in me.location.contents) {
      if (thing != me && thing != target) {
        if (functionp(thing.Message)) {
          // render the message for this observer.
          msg = $.ExpandMessage(args, vars);
          thing.Message(msg);
        }
      }
    }
  }
  return 1;
}

[$.lib.funcs]
roll (act, args, me) {
  args = trim(args);
  num = to_int(args);
  if (num && num == args) {
    if (num < 1) num = 1;
    roll = 1 + random(num);
  } else {
    roll = me.RollDice(args);
  }
  me.Message("You roll "+roll+".");
  me.Speak("emote", "rolls "+roll+".", "$^agent$ ");
  return 1;
}

[$.lib.funcs]
useexit (act, args, me) {
  if (!functionp(me.FollowExit)) return "You cannot move.";
  dir = trim(args);
  result = me.FollowExit(dir, 1);
  if (result == 1) return 1; // success.
  if (stringp(result)) {
    return result;
  } else {
    return "You cannot go " + dir + " from here.";
  }
}

[$.verbs.emote]
requires = 1
command (act, args, me) {
  args = replace(trim(args),"#","##");
  if (length(args) < 1) return "What do you want to emote?";
  msg = "#g" + me.Name() + " " + args;
  room = me.location;
  if (room.Broadcast) room.Broadcast(msg, me);
  me.Message(msg);
  return 1;
}

[$.lib.funcs]
act (act, args, me) {
  args = replace(trim(args),"#","##");
  if (length(args) < 1) return "What action do you want to perform?";
  room = me.location || me; // for room actions.
  if (room.Broadcast) room.Broadcast("#y"+args, me);
  return 1;
}

[$.lib.funcs]
emote (act, args, me) {
  args = trim(args);
  if (args) {
    if (!me.location) return "You are in limbo.";
    other = me.location.Present(args);
    if (other) {
      toRoom = replace(act.other[1], "$target$", other.Name());
      toRoom = replace(toRoom, "$t", other.Name());
      me.location.Broadcast("#g"+me.Name()+" "+toRoom, me, other);
      other.Message("#g"+me.Name()+" "+act.other[2]);
      toMe = replace(act.other[0], "$target$", other.Name());
      toMe = replace(toMe, "$t", other.Name());
      me.Message("#gYou "+toMe);
      return 1;
    } else return "There is no "+args+" here.";
  }
  if (me.location) me.location.Broadcast("#g"+me.Name()+" "+act.self[1], me);
  me.Message("#gYou " + act.self[0], me);
  return 1;
}

[$.lib.funcs]
direct (act, args, me, vars) {
  if (length(args) < 1) return "What do you want to " + act.name + "?";
  args = lower_case(args);
  item = me.ParseMatch(args, me);
  if (!item) return "There is no " + args + " here.";
  result = item.Event(act.opt, vars);
  if (result == 1) return 1;
  if (stringp(result)) return result;
  return "You cannot " + act.name + " " + item.TheName() + ".";
}

[$.lib.funcs]
quit (act, args, me) {
  me.DropConnection();
}

// commands -> player commands grouped by class [flags]
// spells -> all known spells.
// emotes -> all emote commands.
// verbs -> all other interaction verbs.

// not so simple:
// get item from sack (resolve sack; resolve item in sack; send get-from)
// put item in|on sack (resolve item; resolve sack; send put-in)

[$.lib.funcs]
indirect (act, args, me, vars) {
  if (length(args) < 1) return "What do you want to " + act.name + "?";
  args = lower_case(args);
  parts = args.split(" "+act.extra+" ");
  first = parts[0];
  second = parts[1];
  contain = me.ParseMatch(second, me);
  if (!contain) return "There is no " + second + " here.";
  item = contain.FindMatch(first, me, me);
  if (!item) return "There is no " + first + " in " + contain.TheName() + ".";
  vars["indirect"] = contain;
  result = item.Event(act.opt, vars);
  if (result == 1) return 1;
  if (stringp(result)) return result;
  return "You cannot " + act.name + " " + item.TheName() + ".";
}


// skill 1-5 [3 unlocks, 5 combos]
// chance to hit: str + 50% dex + 1h_swords
// do they: add stats, add damage roll

// When a player issues a command:
//    who = the player
//     me = the player
// When a player command affects an npc or item:
//    who = the player
//     me = the npc or item (it)
// When an item interacts with a container:
//    who = the player
//     me = the container
//  other = the item (it)
//
// So:
//    who = the player [object] that initiated the activity
//     me = the object executing this command
//     it = the item being manipulated
//   room = the room this activity occurs in

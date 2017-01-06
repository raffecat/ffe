[$.lib.cmd.user]
date = $.lib.cmd.user.date
i = $.lib.cmd.user.i
l = $.lib.cmd.user.l
who = $.lib.cmd.user.users

[$.lib.cmd.user.look]
-> $.lib.basic.command

Command( args, me )
{
    if (length(args)) {
        oThing = this.ParseMatch(args, me);
        if (!oThing) {
            sDesc = me.location.LookItem(args);
            if (sDesc) return sDesc;
            return "There is no " + args + " here.";
        }
        return oThing.Describe();
    }

    // Look around the room
    me.Look();
    return 1;
}

[$.lib.cmd.user.hold]
-> $.lib.basic.command

Command(args, me)
{
    if (!args) return "What do you want to hold?";

    item = this.ParseMatch(args, me);
    if (!item) return "There is no " + args + " here.";
    if (item.location != me) return "You do not have the " + item.TheName() + ".";

    result = item.Hold(me);
    if (result != 1 )
    {
        if (stringp(result)) return result;
        return "You cannot hold " + item.TheName() + ".";
    }

    me.location.Broadcast( "$agent$ holds $1$.", me, item );

    return "You hold " + item.TheName() + ".";
}

[$.lib.cmd.user.kill]
-> $.lib.basic.command

Command( args, me )
{
    if (length(args) < 1) return "What do you wish to cut to bits?";
    if (!(here = me.location)) return "You are in limbo.";

    oTarget = this.ParseMatch(args, me);
    if (!oTarget) return "There is no " + args + " here.";

    // Check if the target can be attacked
    if (!oTarget.CanBeAttacked) return "You cannot attack " + args + ".";
    if ((ret = oTarget.CanBeAttacked( me )) != 1 )
    {
        if (stringp(ret)) return ret;
        return "You cannot attack " + args + ".";
    }

    if (! me.Attack( oTarget ) )
        return "You are already attacking " + oTarget.TheName() + ".";

    return 1;
}

[$.lib.cmd.user.open]
-> $.lib.basic.command

Command( args, me )
{
    if (length(args) < 1) return "What do you wish to open?";
    if (!(here = me.location)) return "You are in limbo.";

    pos = search( args, " to " );
    if (pos > -1 )
    {
        // Split at the first space
        sThing = args[0..pos - 1];
        sWhere = args[pos + 4..];
    }
    else
    {
        sThing = args;
        sWhere = "";
    }

    item = this.ParseMatch(sThing, me);
    if (!item) return "There is no " + sThing + " here.";

    if (item.Open ) result = item.Open( sWhere, me ); else result = null;
    if (result != 1 )
    {
        if (stringp(result)) return result;
        return "You cannot open " + item.TheName() + ".";
    }

    return "You open " + item.TheName() + ".";
}

[$.lib.cmd.user.follow]
-> $.lib.basic.command

Command( args, me )
{
    if (length(args) < 1) return "Who do you wish to follow?";
    if (!(here = me.location)) return "You are in limbo.";

    oTarget = this.ParseMatch(args, me);
    if (!oTarget) return "There is no " + args + " here.";

    if (oTarget.IsFollowing( me, me ) )
        return "You are already following " + oTarget.TheName() + ".";

    result = oTarget.TestFollow( me, me );
    if (result != 1 )
    {
        if (stringp(result)) return result;
        return "You cannot follow " + oTarget.TheName() + ".";
    }

    oTarget.AddFollower( me );

    return "You begin following " + oTarget.TheName() + ".";
}

[$.lib.cmd.user.get]
-> $.lib.basic.command

Command( args, me )
{
    if (length(args) < 1) return "What do you wish to take?";
    if (!(here = me.location)) return "You are in limbo.";

    // Split into item and container
    pos = search( args, " from " );
    if (pos > -1 )
    {
        // Split at the first space
        item = args[0..pos - 1];
        sContain = args[pos + 4..];

        oContain = this.ParseMatch(sContain, me);
        if (!oContain) return "There is no " + sContain + " here.";

        item = oContain.FindMatch( item, null, me );
        if (!item) return "There is no " + item + " in " + oContain.TheName() + ".";

        result = item.Get( me );
        if (result != 1 )
        {
            if (stringp(result)) return result;
            return "You cannot take " + item.TheName() + ".";
        }

        result = item.Move( me, "$agent$ take*s $1$ from $2$", null, me, [item, oContain] );
        if (result != 1 )
        {
            if (stringp(result)) return result;
            return "You cannot take " + item.TheName() + ".";
        }

        return "You take " + item.TheName() + " from " + oContain.TheName() + ".";
    }
    else
    {
        item = this.ParseMatch(args, me);
        if (!item) return "There is no " + args + " here.";

        result = item.Get( me );
        if (result != 1 )
        {
            if (stringp(result)) return result;
            return "You cannot take " + item.TheName() + ".";
        }

        result = item.Move( me, "$agent$ take*s $1$.", null, me, item );
        if (result != 1 )
        {
            if (stringp(result)) return result;
            return "You cannot take " + item.TheName() + ".";
        }

        return "You pick up " + item.TheName() + ".";
    }
}

[$.lib.cmd.user.browse]
-> $.lib.basic.command

Command( args, me )
{
    if (length(args) < 1) return "What do you wish to browse?";
    if (!(here = me.location)) return "You are in limbo.";

    item = this.ParseMatch(args, me);
    if (!item) return "There is no " + args + " here.";

    if (item.Browse ) result = item.Browse( "", me ); else result = null;
    if (result != 1 )
    {
        if (stringp(result)) return result;
        return "You cannot browse through " + item.TheName() + ".";
    }

    return "You browse through " + item.TheName() + ".";
}

[$.lib.cmd.user.close]
-> $.lib.basic.command

Command( args, me )
{
    if (length(args) < 1) return "What do you wish to close?";
    if (!(here = me.location)) return "You are in limbo.";

    item = this.ParseMatch(args, me);
    if (!item) return "There is no " + args + " here.";

    if (item.Close ) result = item.Close( "", me ); else result = null;
    if (result != 1 )
    {
        if (stringp(result)) return result;
        return "You cannot close " + item.TheName() + ".";
    }

    return "You close " + item.TheName() + ".";
}

[$.lib.cmd.user.give]
-> $.lib.basic.command

Command( args, me )
{
    if (length(args) < 1) return "What do you wish to give?";
    if (!(here = me.location)) return "You are in limbo.";

    // Split into item and container
    pos = search( args, " to " );
    if (pos > 0) {
        item = args[0..pos - 1];
        target = args[pos + 4..];
    } else {
        item = args;
        target = "";
    }

    item = this.ParseMatch(args, me);
    if (!item) return "You do not have '" + args + "'.";
    if (item.location != me) return "You do not have " + item.TheName() + ".";


        oContain = this.ParseMatch(sContain, me);
        if (!oContain) return "There is no " + sContain + " here.";

        item = oContain.FindMatch( item, null, me );
        if (!item) return "There is no " + item + " in " + oContain.TheName() + ".";

        result = item.Get( me );
        if (result != 1 )
        {
            if (stringp(result)) return result;
            return "You cannot take " + item.TheName() + ".";
        }

        result = item.Move( me, "$agent$ take*s $1$ from $2$", null, me, [item, oContain] );
        if (result != 1 )
        {
            if (stringp(result)) return result;
            return "You cannot take " + item.TheName() + ".";
        }

        return "You take " + item.TheName() + " from " + oContain.TheName() + ".";
    }
    else
    {
        item = this.ParseMatch(args, me);
        if (!item) return "There is no " + args + " here.";

        result = item.Get( me );
        if (result != 1 )
        {
            if (stringp(result)) return result;
            return "You cannot take " + item.TheName() + ".";
        }

        result = item.Move( me, "$agent$ take*s $1$.", null, me, item );
        if (result != 1 )
        {
            if (stringp(result)) return result;
            return "You cannot take " + item.TheName() + ".";
        }

        return "You pick up " + item.TheName() + ".";
    }
}

[$.lib.cmd.user.drop]
-> $.lib.basic.command

Command( args, me )
{
    args = trim(args);
    if (!args) return "What do you want to drop?";
    if (!(here = me.location)) return "You are in limbo.";

    item = this.ParseMatch(args, me);
    if (!item) return "You do not have '" + args + "'.";
    if (item.location != me) return "You do not have " + item.TheName() + ".";

    result = item.Drop(me);
    if (result != 1) {
        if (stringp(result)) return result;
        return "You cannot drop " + item.TheName() + ".";
    }

    result = item.Move(here, me.Name()+" drops "+item.AName()+".", null, me, item);
    if (result != 1) {
        if (stringp(result)) return result;
        return "You cannot drop " + item.TheName() + ".";
    }

    return "You drop " + item.TheName() + ".";
}

[$.lib.cmd.user.chat]
-> $.globals
chatsCmd = $.lib.cmd.user.chats
Command( args, me )
{
    if (args) {
        if (!me.creator ) args = replace( args, "#", "##" );

        if (args[0..0] == "@" ) {
            // emote
            result = "#G[chat] " + me.name + " " + args[1..];
            arAppend = [ [ me, args[1..], time(), 1 ] ];
        } else {
            // say
            result = "#G[chat] " + me.name + ":#n " + args;
            arAppend = [ [ me, args, time(), 0 ] ];
        }

        // Add this to the chat history
        if (length(this.history) < 1000 ) this.history += arAppend;
        else this.history = this.history[-999..] + arAppend;

        // Broadcast the chat
        this.loginDaemon.Broadcast( result );
        return 1;
    } else {
        // Display chat history.
      return this.chatsCmd.Command("30 all", me);
    }
}

[$.lib.cmd.user.chats]
-> $.globals
chatCmd = $.lib.cmd.user.chat
Command(args, me) {
  args = split(args," ",0,1);
  num = to_int(args[0]);
  if (!num || num < 1) num = 100;
  history = this.chatCmd.history;
  start = length(history) - num;
  if (start < 0) start = 0;
  today = ctime(time())[0..10];
  repeat {
    lines = [];
    end = start + 10;
    if (args[1] == "all") end = length(history);
    foreach (item in history[start..end-1]) {
        if (item[0]) name = item[0].name;
        else name = "(deleted)";
        ts = ctime(item[2])[0..15];  // strip off seconds and year.
        ts = replace(ts, today, ""); // suppress today's date.
        ts = replace(ts,"  "," ");   // for single-digit dates.
        if (item[3]) {
          append(lines, ts+" #G"+name+" "+item[1]+"#n");
        } else {
          append(lines, ts+" #G"+name+":#n "+item[1]+"#n");
        }
    }
    me.Message(join(lines,"\n"));
    start = end;
    if (start >= length(history)) return 1;
    // use this pattern to invoke the pager.
    res = this.Pager(me);
    if (res) return res; // stop.
  }
  return 1;
}


[$.lib.cmd.user.distribute]
-> $.lib.basic.command

Command ( args, me )
{

  con = me.con;
  int = me.int;
  points = me.levelPoints;
  result = "#m++++++++++++++++++++++++++++++++++++++++++++++#n\n\n";

  if ( length( args ) )
  {
    pos = search( args, " " );
//    if (pos > -1 )
//    {
      nReq = trim(args[0..pos - 1]);
      sWhat = trim(args[pos + 1..]);
//    }

    if (nReq > points )
    {
      result += "Sorry, you dont have " + nReq + " points to spend.\n\n";
    }
  }

  else
  {
    result += "You have gained #Y" + points + "#n points which you can put\n";
    result += "into either your magic or your health, to do this\n";
    result += "type:\n\n";
    result += "#Ydistribute <number of points> magic#n\nor\n";
    result += "#Ydistribute <number of points> health#n\n\n";
  }

  result += "#m++++++++++++++++++++++++++++++++++++++++++++++#n\n";
   me.Message( result );
}



[$.lib.cmd.user.family]
-> $.lib.basic.command

Command( args, me )
{
 result = "";

 if ( args == "" )
 {
if (me.family )
   {
     result += "You are a member of the #Y" + me.family.name + "#n family.\n\n";
     result += "Valid command options are:\n";
     result += "family #cdisown#n <member>, family #cleave#n.\n\n";
     result += "family " + me.family.name + " has the following members:\n";
     foreach(member in me.family.trueMembers)
     {
       result += "#Y" + member.name + "#n\n";
     }
   }
   else
   {
     result += "You are not a member of any family.\n\n";
     result += "You can ask to join an existing family, or create your own with:\n";
     result += "family #ccreate#n <family name>\n";
   }
 }

 else
 {
        pos = search( args, " " );
        if (pos > -1 )
        {
            sOption = args[0..pos - 1];
            args = args[pos + 1..];
        }
        if ( sOption == "create" )
        {
     if ( me.family == null )
     {

          result += "Congratulations,\n You have created the family #Y";
          result += args + "#n\n";
     }
     else result += "You already have a family.\n";
        }

 }

 me.Message( result );

return 1;
}


[$.lib.cmd.user.finger]
-> $.lib.basic.command
users = $.save.user

Command ( args, me )
{
  sStarLine = "#m++++++++++++++++++++++++++++++++++++++++++++++#n";
  user = this.loginDaemon.GetUser(args);
  result = "";

  result += sStarLine + "\n";
  if ( objectp(user) )
  {
   idle = time() - user.lastInput;
   isIdle = idle;

   days = to_int(idle / 86400);
   daySub = days * 86400;
   idle = to_int(idle - daySub);

   hrs = to_int(idle / 3600);
   hrsSub = hrs * 3600;
   idle = to_int(idle - hrsSub);

   mins = to_int(idle / 60);
   minSub = mins * 60;
   idle = to_int(idle - minSub);


   if (!user.family == null )
   {
    family = user.family.name;
   }
   else
   {
    family = "of " + user.race.homeTown;
   }

    result += "#YName#n       : " + user.name + " " + family + "\n";
    result += "#YGender#n     : " + user.gender + "\n";
    result += "#YRace#n       : " + user.race.name + "\n";
    result += "#YBackground#n : " + user.class.name + "\n";

   if (user.creator == 1 )
   {
     result += user.name + " is a #Ycreator.#n\n";
   }
   if (user.lord == 1 )
   {
     if (user.gender == "male" )
     {
       result += user.name + " is a #CDomain Lord.#n\n";
     }
     if (user.gender == "female" )
     {
       result += user.name + " is a #CDomain Lady.#n\n";
     }
     if (user.gender == "neuter" )
     {
       result += user.name + " is a #CDomain Leader.#n\n";
     }
   }

   if (user.froggy == 1 )
   {
     result += user.name + " is also a #GFroggy#n creator, a guest.#n\n";
   }

   if (user.admin == 1 )
   {
     result += user.name + " is an #Radministrator.#n\n";
   }

    result += user.name + " is ";

    if(days >= 1) { result += days + " days "; }
    if(hrs >= 1) { result += hrs + " hours "; }
    if(mins >= 1) { result += mins + " minutes "; }
    if(idle >= 1) { result += "and " + idle +" seconds "; }
    if(!isIdle == 0) { result += "idle."; }
    if(isIdle == 0) { result += "not idle at all.\n"; }
    result += "\n#YPlan#n       :\n" + user.plan + "\n";

       result += "\n" + sStarLine + "\n";

   if (me.admin == 1 )
   {
    result += "#RAs an admin you know the following.#n\n" + sStarLine + "\n";
    if(user.creator == 1)
    {
      result += "#RWorking dir#n     :\n"; // user.cwd not print?
    }
    if (user.location == null )
    {
      result += "#RLocation#n        : Not Logged in, starts at " + user.startLocation.name + "\n";
    }
    else
    {
      result += "#RLocation#n        : " + user.location.name + "\n";
    }
      result += "#REmail#n           : " + user.email + "\n";
      result += "#RReal Name#n       : " + user.realName + "\n";
      result += "\n" + sStarLine + "\n";
   }

   }
   else
   {
    result += "Sorry this person is not on our records.";
    result += "\n" + sStarLine + "\n";
   }


   me.Message( result );
}


[$.lib.cmd.user.alias]
-> $.lib.basic.command

help = "Add command aliases.\n"
       "\n"
       "Usage: alias <name> <command>\n"
       "       alias <name> -\n"
       "\n"
       "Your aliases will only match the first word of a command. "
       "Aliases can use other aliases up to 10 times.\n"
       "\n"
       "Use $$ to insert the rest of the line into the aliased command.\n"
       "\n"
       "Examples: alias tp tell pomke\n"
       "          alias twr emote twirls $$ around.\n"
       "\n"
       "In the first example, 'tp hello' will 'tell pomke hello'\n"
       "In the second example, 'twr Pomke' will 'emote twirls Pomke around.'\n"
       "\n"

[$.lib.cmd.user.alias]
Command(args, me) {
  if (args == "help") return this.Help(me);
  args = split(args, " ", 1, 1);
  name = args[0];
  cmd = args[1];
  if (!name) {
    aliases = me.aliases;
    if (aliases) {
      s = "You have the following aliases:\n\n";
      foreach(alias in aliases) {
        s += alias+" -> "+aliases[alias]+"\n";
      }
    } else {
      s = "You have not set any aliases.\n";
    }
    me.RawMessage(s+"\nEnter 'alias help' for help.\n");
    return 1;
  }
  if (!cmd) {
    aliases = me.aliases;
    found = 0;
    s = "You have the following aliases matching '"+name+"':\n\n";
    foreach(alias in aliases) {
      if (search(alias, name) >= 0) {
        s += alias+" -> "+aliases[alias]+"\n";
        found = 1;
      }
    }
    if (!found) {
      s = "You do not have any aliases matching '"+name+"'.\n";
    }
    me.RawMessage(s+"\nEnter 'alias help' for help.\n");
    return 1;
  } else if (cmd == "-") {
    alias = me.aliases[name];
    if (alias) {
      remove(me.aliases, name);
      me.RawMessage("Removed alias '"+name+"'.\n");
      return 1;
    } else {
      return "You do not have an alias named '"+name+"'.";
    }
  } else {
    if (!me.aliases) me.aliases = new mapping;
    old = me.aliases[name];
    me.aliases[name] = cmd;
    if (old) me.RawMessage("Replaced alias '"+name+"'.\n");
    else me.RawMessage("Added alias '"+name+"'.\n");
    return 1;
  }
}


[$.lib.cmd.user.help]
-> $.lib.basic.command
helpDir = $.lib.help
lists = [["Player commands", $.lib.cmd.user], ["Immortal commands", $.lib.cmd.imm], ["Admin commands", $.lib.cmd.admin]]

pastet $.lib.cmd.user.help.help-list
{each list in lists}
#m |
#m-+----------------
#m |#Y {list[0]}:
#m-+------------------------------------
{each cmd in list[1]}{cols 13}#m |#n {cmd.id}{end cols}{end each}{end each}

[$.lib.cmd.user.help]
Command(args, me) {
  vars = new mapping;
  vars["lists"] = this.lists;
  me.MessageNoWrap(this.RenderTemplate(this.templateCache["help-list"], vars, me));
  return 1;
}


[$.lib.cmd.user.i]
-> $.lib.basic.command

Command( args, me )
{
    sResult = "";

    // Build the array of visible items
    remove( me.contents, null );
    arContents = me.contents;
    arItems = [];
    arWorn = [];
    arHeld = [];
    foreach( oThing in arContents )
    {
        if (oThing.visible )
        {
            if (oThing.worn ) arWorn += [ oThing ];
            else if (oThing.held ) arHeld += [ oThing ];
            else arItems += [ oThing ];
        }
    }

    items = this.ListItems( arHeld, me );
    if (length(items) ) sResult += "#oYou are holding: #n" + items + ".\n";
    else sResult += "#oYou are empty-handed.#n\n";

    items = this.ListItems( arItems, me );
    if (length(items) ) sResult += "#gYou are carrying: #n" + items + ".\n";
    else sResult += "#gYou are carrying nothing.#n\n";

    items = this.ListItems( arWorn, me );
    if (length(items) ) sResult += "#rYou are wearing: #n" + items + ".";
    else sResult += "#rYou are completely naked.#n";

    return sResult;
}


[$.lib.cmd.user.option]
-> $.lib.basic.command

Command( args, me )
{
    if (args == "") {

        thingy  = "#m |#n\n";
        thingy += "#m-+----------------#n\n";
        thingy += "#m | #C" + me.name + "'s Options#n\n";
        thingy += "#m-+----------------------------------#n\n";
        thingy += "#m |#n\n";

        foreach(opt in me.option) {
            thingy += "#m |#Y "+opt+"#n: " + me.option[opt] + "\n";
        }
    }

    return thingy;
}


[$.lib.cmd.user.reply]
-> $.lib.basic.command

Command( args, me )
{
    if (!me.tellReply) return "No one is talking to you.";

    if (args )
    {
        sName = me.tellReply.name;

        // Find the user
        oUser = this.loginDaemon.FindUser( sName );
        if (!oUser) return "User is not logged on: " + sName;

        args = replace( args, "$", "$$" );
        if (!me.creator ) args = replace( args, "#", "##" );

        // Send the tell to the user
        me.SendTell( oUser, args );

        return 1;
    }

    return "You are talking to #Y" + me.tellReply.name + "#n.";
}


[$.lib.cmd.user.tell]
-> $.lib.basic.command

Command( args, me )
{
    if (args )
    {
        // Split into name and message
        pos = search( args, " " );
        if (pos > -1 )
        {
            // Split at the first space
            sName = args[0..pos - 1];
            args = args[pos + 1..];

            // Find the user
            oUser = this.loginDaemon.FindUser( sName );
            if (!oUser) return "User is not logged on: " + sName;
            args = replace( args, "$", "$$" );
            if (!me.creator ) args = replace( args, "#", "##" );
            // Send the tell to the user
            me.SendTell( oUser, args );
            return 1;
        }

        return "Usage: tell <user> <stuff>";
    }

    me.DisplayTellHistory();
    return 1;
}


[$.lib.cmd.user.date]
-> $.lib.basic.command

Command( args, me )
{

 hour = $.lib.daemon.worldsys.hour;
 day = $.lib.daemon.worldsys.dayName;
 moon = $.lib.daemon.worldsys.monthName.name;
 stone = $.lib.daemon.worldsys.monthName.stone;
 animal = $.lib.daemon.worldsys.monthName.animal;
 tree = $.lib.daemon.worldsys.monthName.tree;
 year = $.lib.daemon.worldsys.year;
 starline = "#m++++++++++++++++++++++++++++++++++++++++++#n";
 s = starline + "\n\n";
 s += "It is near " + hour + ":00 on " + day + ".\n\n";
 s += "#YOf the moon#n     : " + moon + ".\n";
 s += "#YGuardian tree#n   : The " + tree + ".\n";
 s += "#YGuardian spirit#n : The " + animal + ".\n";
 s += "#YRuling stone#n    : The " + stone + ".\n\n";
 s += starline + "\n";
 me.Message( s );
}

[$.lib.cmd.user.title]
-> $.lib.basic.command

Command( args, me )
{
    if (args == "" )
    {
        return "What would you like your title to be?";
    }
    else
    {
        args = replace( args, "$", "$$" );
        //if (!me.creator ) args = replace( args, "#", "##" );

        me.title = args;
        return "#YTitle set to:#n " + args;
    }
}

[$.lib.cmd.user.unfollow]
-> $.lib.basic.command

Command( args, me )
{
    if (length(args) < 1) return "Who do you wish to stop following?";
    if (!(here = me.location)) return "You are in limbo.";

    args = lower_case(args);
    oTarget = here.FindMatch( args, null, me );
    if (!oTarget) return "There is no " + args + " here.";

    if (!oTarget.IsFollowing( me, me ) )
        return "You are not following " + oTarget.TheName() + ".";

    oTarget.RemoveFollower( me );

    return "You stop following " + oTarget.TheName() + ".";
}

[$.lib.cmd.user.users]
-> $.lib.basic.command

Command( args, me )
{
    family = "";
    starline = "#m" + me.RepeatString( "+", me.cols - 3 ) + "#n\n";
    mudname = "#C " + me.mudName + " #m";
    s = "";
    number = 0;
    foreach( user in this.loginDaemon.Users() ) {
        sIdle = (time() - user.lastInput) / 60 + "min";
        sFlags = "";

        if (user.admin == 1 ) {
          sFlags += " #RA#n ";
          userType = 1;
          }

          else if (user.lord == 1 ) {
          sFlags += " #WD#n ";
          }

        else if (user.creator == 1 ) {
          sFlags += " #YI#n "; // imm
          }

        else {
          sFlags += "#R1#n  ";
          }
        if (user.race ) {
          sFlags += "#y " + user.race.name;
          }
        else sFlags += "#y ???";

        if (user.class ) {
          sFlags += "#y " + user.class.name;
          }
        else sFlags += "#y ???";

        if (user.family )
        {
          family = user.family.name;
        }
        else if ( user.race )
        {
          family = "of " + user.race.homeTown;
        }
        s += "[" + sFlags + "#n ] #C" + user.name + " " + family +
             "#n " + user.title + "#n  " + /* sIdle +*/ "\n";
        number = number + 1;
    }

    if (number == 1 ) {
        s += "\nThere is only you, alone in the dark...\n";
    } else {
        s += "\n There are currently " + number + " users online\n";
    }

    result = starline + s + starline;
    me.Message( result );
    return 1;
}


[$.lib.cmd.user.whoswho]
-> $.lib.basic.command

Command( args, me )
{
    users = $.save.user;
    s = "";
    foreach( username in users )
    {
        user = users[username];
        s += "#C" + user.name + "#n is rly: " + username + "\n";
    }
    me.Message( s );
}



[$.lib.cmd.user.post]
-> $.lib.basic.command

Command( args, me )
{
    if (length(args) < 1) return "What do you wish to post in?";
    if (!(here = me.location)) return "You are in limbo.";

    args = lower_case(args);
    if (args[0..2] == "in " ) args = args[3..]; // optional

    item = this.ParseMatch(args, me);
    if (!item) return "There is no " + args + " here.";

    if (item.PostIn ) result = item.PostIn( "", me ); else result = null;
    if (result != 1 )
    {
        if (stringp(result)) return result;
        return "You cannot post a note in " + item.TheName() + ".";
    }

    return "You post a note in " + item.TheName() + ".";
}

[$.lib.cmd.user.put]
-> $.lib.basic.command

Command( args, me )
{
    if (length(args) < 1) return "What do you wish to put, and where?";
    if (!(here = me.location)) return "You are in limbo.";

    // Split into item and container
    args = lower_case(args);
    pos = search( args, " in " );
    if (pos > -1 )
    {
        // Split at the first space
        item = args[0..pos - 1];
        sContain = args[pos + 4..];
    }
    else
    {
        return "Usage: put <item> in <container>";
    }

    item = this.ParseMatch(item, me);
    if (!item) return "There is no " + item + " here.";

    oContain = this.ParseMatch(sContain, me);
    if (!oContain) return "There is no " + sContain + " here.";

    result = item.Put( oContain, me );
    if (result != 1 )
    {
        if (stringp(result)) return result;
        return "You cannot put " + item.TheName() + " in " + oContain.TheName() + ".";
    }

    result = item.Move( me, "$agent$ put*s $1$ in $2$.", null, me, [item, oContain] );
    if (result != 1 )
    {
        if (stringp(result)) return result;
        return "You cannot put " + item.TheName() + " in " + oContain.TheName() + ".";
    }

    return "You put " + item.TheName() + " in " + oContain.TheName() + ".";
}

[$.lib.cmd.user.read]
-> $.lib.basic.command

Command( args, me )
{
    if (length(args) < 1) return "What do you wish to read?";
    if (!(here = me.location)) return "You are in limbo.";

    sWhat = "";
    if (to_int(args) )
    {
        sWhat = args;
        args = "diary"; // hack =)
    }

    item = this.ParseMatch(args, me);
    if (!item) return "There is no " + args + " here.";

    if (item.Read ) result = item.Read( sWhat, me ); else result = null;
    if (result != 1 )
    {
        if (stringp(result)) return result;
        return "You cannot read " + item.TheName() + ".";
    }

    return "You read " + item.TheName() + ".";
}

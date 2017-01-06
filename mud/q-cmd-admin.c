[$.lib.cmd.admin.pastet]
-> $.lib.basic.command
usage = "Usage: pastet <path>";

Command(args, me) {
  words = split(args," ",0,1);
  if (!words[0]) return this.usage;
  parts = this.FindPathName(words[0],$);
  obj = parts[0];
  name = parts[1];
  if (!obj || !name) return this.usage;
  // read lines.
  lines = [];
  repeat {
    me.RawMessage(">");
    line = read_line(me.loginSocket);
    if (line == ".") break;
    lines += [ line ];
  }
  if (!lines) return "No lines entered.";
  // compile template.
  src = join(lines, "\n");
  warn = null; if (words[1]) warn = me.RawMessage;
  func = this.CompileTemplate(src, warn);
  if (stringp(func)) return "Template did not compile.";
  if (!obj.templates) obj.templates = new mapping;
  if (!obj.templateCache) obj.templateCache = new mapping;
  obj.templates[name] = lines;
  obj.templateCache[name] = func;
  me.Message("Set template '"+name+"' of "+object_path(obj));
  return 1;
}


[$.lib.cmd.admin.import]
-> $.lib.basic.command

Command(args, me) {
    // Allow the user to paste a buffer full of text.
    buf = "";
    repeat {
        me.RawMessage(">");
        line = read_line(me.loginSocket);
        if (line == ".") break;
        buf += line + "\n";
    }
    // Import the buffer into the mud.
    errors = parse(buf, 0);
    if (stringp(errors)) {
        me.MessageNoWrap("#C"+replace(errors,"#","##")+"#n");
        return 1;
    } else {
        return "Done.";
    }
}


[$.lib.cmd.admind.cp]
-> $.lib.basic.command

Command( sArgs, oCaller )
{
    if( length(sArgs) )
    {
        // Split the sArgs into source and destination
        nPos = search( sArgs, " " );
        if( nPos > -1 )
        {
            // Split at the delimiter
            sSource = trim(sArgs[0..nPos - 1]);
            sDest = sArgs[nPos + 1..];

            // Find the source object
            arTarget = this.FindPathName( sSource, oCaller.cwd );
            if( !arTarget ) return "Not found: " + sSource;
            sSourceName = arTarget[1];
            oSource = arTarget[0];

            // Check if the source attribute exists
            if( !exists( oSource, sSourceName ) )
            {
                return "Not found: " + sSource;
            }

            mValue = oSource[sSourceName];

            if( objectp(mValue) )
            {
                // Check if the target object exists
                oDest = find_object( sDest, oCaller.cwd );
                if( oDest )
                {
                    if( !this.AskYesNo( oCaller, "Already exists, overwrite attributes (y/n)? " ) )
                        return 1;
                }
                else
                {
                    // Check if an attribute exists at the destination
                    arTarget = this.FindPathName( sDest, oCaller.cwd );
                    if( arTarget && exists( arTarget[0], arTarget[1] ) )
                    {
                        if( !this.AskYesNo( oCaller, "Already exists, overwrite (y/n)? " ) )
                            return 1;
                    }

                    // Create a new object
                    oDest = new object( sDest, oCaller.cwd );
                }

                // Copy object inheritance
                foreach( oInherit in inherit_list(mValue) )
                {
                    add_inherit( oDest, oInherit );
                }

                // Copy the object attributes (exclude child objects)
                foreach( sAttrib in mValue )
                {
                    mVal = mValue[sAttrib];
                    if( !childp(mVal) )
                    {
                        oDest[sAttrib] = mVal;
                    }
                }
            }
            else
            {
                // Find the destination object
                arTarget = this.FindPathName( sDest, oCaller.cwd );
                if( !arTarget ) return "Not found: " + sDest;
                sDestName = arTarget[1];
                oDest = arTarget[0];

                // Check if the target exists
                if( exists( oDest, sDestName ) )
                {
                    if( !this.AskYesNo( oCaller, "Already exists, overwrite (y/n)? " ) )
                        return 1;
                }

                // Copy the attribute
                oDest[sDestName] = mValue;
            }

            return 1;
        }
    }

    return "Usage: cp <source> <destination>";
}


[$.lib.cmd.admin.ed]
-> $.lib.basic.command

Command( sArgs, oCaller )
{
    mValue = null;
    sName = null;

    if( length(sArgs) )
    {
        // Find the target string or function
        arTarget = this.FindPathName( sArgs, oCaller.cwd );
        if( !arTarget ) return "Not found: " + sArgs;
        oTarget = arTarget[0];
        sName = arTarget[1];

        if( exists(oTarget, sName) )
        {
            mValue = oTarget[sName];

            // Determine the type
            bIsFunction = functionp(mValue);
            if( bIsFunction ) mValue = encode(mValue);
            if( !stringp(mValue) ) return "Not a string or function: " + sArgs;
        }
    }

    if( !mValue )
    {
        // Create a new string
        if( !sName ) sName = "new";
        oCaller.Message( "Creating string: " + sName );
        mValue = "";
        bIsFunction = 0;
    }

    // Open the 'edit' buffer
    mResult = oCaller.EditOpenBuffer( "edit", mValue, sArgs, bIsFunction );
    if( mResult != 1 ) return mResult;

    // Accept commands until the editor is closed
    while( oCaller.EditIsOpen( "edit" ) )
    {
        sCmd = trim( oCaller.ReadLine( ": " ) );
        mResult = oCaller.EditCommand( "edit", sCmd, oCaller );
        if( stringp(mResult) ) oCaller.Message( mResult );
        if( !mResult ) oCaller.Message( "Invalid edit command." );
    }
}

[$.lib.cmd.admin.eval]
-> $.lib.basic.command

Command(args, me)
{
    args = trim(args);
    if (!args) return "Usage: eval <expression>";

    // Attempt to compile the expression into a function.
    result = parse("function() { return ("+args+"); }", 1);
    if (functionp(result)) {
        // Functions must be bound to execute them.
        me.tempEvalFunction = result;
        me.RawMessage(to_string(me.tempEvalFunction())+"\n");
        return 1;
    } else if (stringp(result)) {
        me.MessageNoWrap("#C"+replace(result,"#","##")+"#n");
        return 1;
    } else {
        return "Expression did not compile.";
    }
}

[$.lib.cmd.admin.exec]
-> $.lib.basic.command

Command(args, me)
{
    args = trim(args);
    if (!args) return "Usage: exec <expression>";

    // Attempt to compile the expression into a function.
    if (args[-1] != ";") args += ";";
    result = parse("function() { "+args+" }", 1);
    if (functionp(result)) {
        // Functions must be bound to execute them.
        me.tempEvalFunction = result;
        me.RawMessage(to_string(me.tempEvalFunction())+"\n");
        return 1;
    } else if (stringp(result)) {
        me.MessageNoWrap("#C"+replace(result,"#","##")+"#n");
        return 1;
    } else {
        return "Statements did not compile.";
    }
}

[$.lib.cmd.admin.inherit]
-> $.lib.basic.command

Command( sArgs, oCaller )
{
    oTarget = oCaller.cwd;

    if( length(sArgs) )
    {
        // Find the target object
        oInherit = find_object( sArgs, oCaller.cwd );
        if( !oInherit ) return "Not found: " + sArgs;
        add_inherit( oTarget, oInherit );

        return "Added inherit: " + replace( object_path( oInherit ), "$", "$$" );
    }

    return "Usage: inherit <object>";
}

[$.lib.cmd.admin.ls]
-> $.lib.basic.command
Command( sArgs, oCaller )
{
    // List attributes in current object
    nCell = 16;
    nCols = oCaller.cols;

    bSuffix = 0;
    if (sArgs[0..1] == "-F") { bSuffix = 1; sArgs = sArgs[3..]; }

    if (length(sArgs)) {
        // Find the specified object
        ob = find_object(sArgs, oCaller.cwd);
        if (!ob) return "not found: " + sArgs;
    } else {
        // Use the current working directory
        ob = oCaller.cwd;
    }

    // Build the object header
    if (ob == $) result = "#K@";
    else result = "#K" + replace(object_path(ob),"$.","@");
    bases = inherit_list(ob);
    if (bases) {
        result += " -> ";
        first = 1;
        foreach (base in bases) {
            if (first) first=0; else result += ", ";
            result += replace(object_path(base),"$.","@");
        }
    }
    result += "#n\n";

    arResult = [];

    // List all the objects
    foreach (name in keys(ob)) {
        val = ob[name];
        if (childp(val)) {
            if (inherit_list(val)) {
                // Object is a room/item/npc/player
                if (bSuffix) arResult += [ "#m" + name + "*" ];
                else arResult += [ "#m" + name ];
            } else {
                // Object is a directory
                if (bSuffix) arResult += [ "#g" + name + "/" ];
                else arResult += [ "#g" + name ];
            }
        }
    }

    // Add the list of attributes
    foreach (name in keys(ob)) {
        val = ob[name];
        if (!objectp(val)) {
            col = "";
            if (objectp(val)) col = "#c";
            else if (functionp(val)) col = "#o";
            sym = "";
            if (bSuffix) {
                if (integerp(val)) sym="%";
                else if (realp(val)) sym="~";
                else if (rangep(val)) sym="-";
                else if (stringp(val)) sym="$";
                else if (arrayp(val)) sym="[]";
                else if (mappingp(val)) sym="{}";
                else if (objectp(val)) sym="@";
                else if (functionp(val)) sym="()";
                else if (portp(val)) sym="!";
                else if (threadp(val)) sym="&";
            }
            arResult += [ col + name + sym ];
        }
    }

    nCurrent = 0;
    foreach (sItem in arResult) {
        nLen = length(sItem) + 1; // 1 for trailing space
        if( sItem[0..0] == "#" ) nLen -= 2; // adjust for colour
        if( nCurrent + nLen >= nCols && nCurrent > 0 ) { result += "\n"; nCurrent = 0; }
        result += sItem + "#n ";
        nCurrent += nLen;
        nPad = nCell - (nLen % nCell);
        if( nPad != nCell ) {
            result += this.RepeatString( " ", nPad );
            nCurrent += nPad;
        }
    }

    oCaller.MessageNoWrap( result + "\n" );
    return 1;
}

[$.lib.cmd.admin.mk]
-> $.lib.basic.command

Command( sArgs, oCaller )
{
    if( length(sArgs) )
    {
        // Find the parent
        oTarget = this.FindPathName( sArgs, oCaller.cwd );
        if( !oTarget ) return "Not found: " + sArgs;
        sName = oTarget[1];
        oTarget = oTarget[0];

        if( exists( oTarget, sName ) )
            return "Already exists: " + sArgs;

        // Create an object with this name
        oNew = new object( sName, oTarget );
        return "Created object: " + replace( object_path( oNew ), "$", "$$" );
    }

    return "Usage: mk <path>";
}


[$.lib.cmd.admin.pwd]
-> $.lib.basic.command

Command( sArgs, oCaller )
{
    // Display current working directory
    oCaller.RawMessage( object_path( oCaller.cwd ) + "\n" );
    return 1;
}

[$.lib.cmd.admin.rm]
-> $.lib.basic.command

Command( sArgs, oCaller )
{
    if( length(sArgs) )
    {
        // Find the parent
        oTarget = this.FindPathName( sArgs, oCaller.cwd );
        if( !oTarget ) return "Not found: " + sArgs;
        sName = oTarget[1];
        oTarget = oTarget[0];

        // Check that the attribute exists
        if( !exists( oTarget, sName ) )
            return "Not found: " + sArgs;

        // Check if the target is an object
        mThing = oTarget[sName];
        if( childp(mThing) )
        {
            // The thing is an actual object
            // Make sure the object is empty before allowing delete
            foreach( sKey in mThing )
            {
                if( childp(mThing[sKey]) )
                    return "Object must be empty: " + sArgs;
            }

            // Delete the object
            delete mThing;
            remove( oTarget, sName );
            return "Deleted object: " + sArgs;
        }

        // Delete the attribute with this name
        remove( oTarget, sName );
        return "Removed attribute: " + sArgs;
    }

    return "Usage: rm [<path>/]<attribute>";
}

[$.lib.cmd.admin.set]
-> $.lib.basic.command

Command(args, me)
{
    args = trim(args);
    pos = search(args, " ");
    if (pos > 0) {
        path = trim(args[0..pos - 1]);
        value = trim(args[pos + 1..]);
        target = this.FindPathName(path, me.cwd);
        if (!target) return "Not found: " + path;
        obj = target[0];
        name = target[1];
        // use the parser to import the value
        s = "["+object_path(obj)+"]\n";
        s += name+" = "+value+"\n";
        result = parse(s,0);
        if (stringp(result)) {
            // parse error
            me.MessageNoWrap("#C"+replace(result,"#","##"));
            return 1;
        } else {
            me.MessageNoWrap("Set '"+name+"' to: "+value+"\n");
            return 1;
        }
    }
    return "Usage: set {path} {value}";
}


[$.lib.cmd.admin.shutdown]
-> $.lib.basic.command

Command( sArgs, oCaller )
{
    if( oCaller.admin )
    {
        // Schedule the shutdown (must be on a non-user thread!)
        thread this.DoShutdown( oCaller );

        // Quit all users
        foreach( oUser in this.loginDaemon.Users() )
        {
            try oUser.Quit();
        }
    }
    else return "You cannot do that.";
}

DoShutdown( oCaller )
{
    // Wait 2 seconds for everyone to quit
    sleep( 2 );

    if( oCaller.admin )
    {
        // Shut down the mud
        shutdown();
    }
}

[$.lib.cmd.admin.threads]
-> $.lib.basic.command

Command( sArgs, oCaller )
{
    arThreads = threads();

    bKill = 0;
    if( sArgs[0..4] == "-kill" )
    {
        nPid = to_int( sArgs[6..] );

        if( nPid == null )
        {
            sArgs = sArgs[6..]; // filter
            if( length(sArgs) < 1 ) return "Must specify a filter.";
            bKill = 1;
        }
        else
        {
            if( nPid < 1 || nPid > length(arThreads) )
                return "Invalid thread id.";

            sArgs = to_string(nPid) + " ";
            bKill = 1;
        }
    }

    sResult = "";
    nPid = 1;
    foreach( thValue in arThreads )
    {
        fnMain = thread_function( thValue );
        nState = thread_state( thValue );

        sName = function_name( fnMain );
        sPath = replace( object_path( function_context( fnMain ) ), "#", "##" );
        if( !sPath ) sPath = "no-context";
        switch( nState )
        {
        case 0: sState = "#Kstopped"; break;
        case 1: sState = "#Yrunning"; break;
        case 2: sState = "#RSLEEP"; break;
        case 3: sState = "#RREAD"; break;
        case 4: sState = "#RREADLN"; break;
        case 5: sState = "#RWRITE"; break;
        case 6: sState = "#RACCEPT"; break;
        default: sState = "#RBLOCKED"; break;
        }

        sReport = nPid + " " + sPath + "." + sName + " [" + sState + "#n]\n";

        // Apply an optional free-text filter
        if( !length(sArgs) || search( sReport, sArgs ) >= 0 )
        {
            if( bKill )
            {
                kill_thread( thValue );
                if( is_stopped( thValue ) ) sResult += "Killed: ";
                else sResult += "Failed to kill: ";
            }
            sResult += sReport;
        }

        nPid += 1;
    }

    if( length(sResult) < 1 )
        sResult = "No threads match the specified filter.";

    sResult =
    oCaller.MessageNoWrap( sResult );

    return 1;
}

[$.lib.cmd.admin.uninherit]
-> $.lib.basic.command

Command( sArgs, oCaller )
{
    oTarget = oCaller.cwd;

    if( length(sArgs) )
    {
        // Find the target object
        oInherit = find_object( sArgs, oCaller.cwd );
        if( !oInherit ) return "Not found: " + sArgs;
        remove_inherit( oTarget, oInherit );

        return "Removed inherit: " + replace( object_path( oInherit ), "$", "$$" );
    }

    return "Usage: uninherit <object>";
}


[$.lib.cmd.admin.makeimm]
-> $.lib.basic.command
users = $.save.user
immCommands = $.lib.cmd.imm

Command ( sArgs, oCaller )
{
    name = lower_case(trim(sArgs));
    if (!name) {
        return "Usage: makeimm <name>";
    }
    if (!exists(this.users, name)) {
        return "No such user: " + name;
    }
    user = this.users[name];
    user.creator = 1;
    user.commands = this.immCommands;
    return user.name + " is now an immortal.";
}

[$.lib.cmd.admin.mv]
-> $.lib.basic.command

Command( sArgs, oCaller )
{
    if( length(sArgs) )
    {
        // Split the sArgs into source and destination
        nPos = search( sArgs, " " );
        if( nPos > -1 )
        {
            // Split at the delimiter
            sSource = trim(sArgs[0..nPos - 1]);
            sDest = trim(sArgs[nPos + 1..]);

            // Find the original object
            oSource = find_object( sSource, oCaller.cwd );
            if (!oSource) {
                return "Not found: " + sSource;
            }

            // Check if an attribute exists at the destination
            arTarget = this.FindPathName( sDest, oCaller.cwd );
            if (arTarget && exists(arTarget[0], arTarget[1])) {
                return "Destination exists: " + sDest;
            }

            move_object(oSource, arTarget[0], arTarget[1]);

            return "Moved " + sSource + " to " + sDest;
        }
    }

    return "Usage: mv <source> <destination>";
}

[$.lib.cmd.admin.cat]
-> $.lib.basic.command

Command( sArgs, oCaller )
{
    // Show attributes and values in current object

    if( length( sArgs ) )
    {
        if( sArgs[0..2] == "-c " ) { sArgs = sArgs[3..]; bColour = 1; } else bColour = 0;

        // Find the specified object
        if( sArgs == "." ) mValue = oCaller.cwd;
        else
        {
            arResult = this.FindPathName( sArgs, oCaller.cwd );
            if( !arResult ) return "Not found: " + sArgs;
            oTarget = arResult[0];
            sName = arResult[1];
            if( !exists( oTarget, sName ) ) return "Not found: " + sArgs;
            mValue = oTarget[sName];
        }

        if( objectp(mValue) )
        {
            // Build the object header
            sResult = "[" + object_path( mValue ) + "]\n";
            arList = inherit_list( mValue );
            if( sizeof(arList) )
            {
                foreach( oInherit in arList )
                {
                    sResult += "-> " + object_path( oInherit ) + "\n";
                }
            }
            sResult += "\n";

            // Add the list of attributes
            foreach( sName in keys( mValue ) )
            {
                mAttrib = mValue[sName];
                if( functionp(mAttrib) )
                {
                    sResult += encode( mAttrib ) + "\n";
                    continue;
                }
            }
            foreach( sName in keys( mValue ) )
            {
                mAttrib = mValue[sName];
                if( objectp(mAttrib) )
                {
                    sResult += sName + " = " + object_path( mAttrib ) + "\n";
                    continue;
                }
                if( !functionp(mAttrib) )
                    sResult += sName + " = " + encode( mAttrib ) + "\n";
            }

            oCaller.RawMessage( sResult );
            return 1;
        }
        else if( stringp(mValue) )
        {
            if( bColour ) oCaller.MessageNoWrap( mValue + "\n" );
            else oCaller.RawMessage( mValue + "\n" );
            return 1;
        }
        else
        {
            oCaller.RawMessage( encode( mValue ) + "\n" );
            return 1;
        }
    }
}

[$.lib.cmd.admin.cd]
-> $.lib.basic.command
home = $.home
root = $

Command( sArgs, oCaller )
{
    // Change current working directory
    if( !length(sArgs) )
    {
        // Change to user home directory
        sUsername = oCaller.username;
        if( exists( this.home, sUsername ) ) oDir = this.home[sUsername];
        else oDir = this.root;
    }
    else
    {
        oDir = find_object( sArgs, oCaller.cwd );
        if( !oDir ) return "not found: " + sArgs;
    }

    oCaller.cwd = oDir;

    oCaller.RawMessage( object_path( oDir ) + "\n" );
    return 1;
}

[$.lib.cmd.admin.cp]
-> $.lib.basic.command

Command( sArgs, oCaller )
{
    if( length(sArgs) )
    {
        // Split the sArgs into source and destination
        nPos = search( sArgs, " " );
        if( nPos > -1 )
        {
            // Split at the delimiter
            sSource = trim(sArgs[0..nPos - 1]);
            sDest = sArgs[nPos + 1..];

            // Find the source object
            arTarget = this.FindPathName( sSource, oCaller.cwd );
            if( !arTarget ) return "Not found: " + sSource;
            sSourceName = arTarget[1];
            oSource = arTarget[0];

            // Check if the source attribute exists
            if( !exists( oSource, sSourceName ) )
            {
                return "Not found: " + sSource;
            }

            mValue = oSource[sSourceName];

            if( objectp(mValue) )
            {
                // Check if the target object exists
                oDest = find_object( sDest, oCaller.cwd );
                if( oDest )
                {
                    if( !this.AskYesNo( oCaller, "Already exists, overwrite attributes (y/n)? " ) )
                        return 1;
                }
                else
                {
                    // Check if an attribute exists at the destination
                    arTarget = this.FindPathName( sDest, oCaller.cwd );
                    if( arTarget && exists( arTarget[0], arTarget[1] ) )
                    {
                        if( !this.AskYesNo( oCaller, "Already exists, overwrite (y/n)? " ) )
                            return 1;
                    }

                    // Create a new object
                    oDest = new object( sDest, oCaller.cwd );
                }

                // Copy object inheritance
                foreach( oInherit in inherit_list(mValue) )
                {
                    add_inherit( oDest, oInherit );
                }

                // Copy the object attributes (exclude child objects)
                foreach( sAttrib in mValue )
                {
                    mVal = mValue[sAttrib];
                    if( !childp(mVal) )
                    {
                        oDest[sAttrib] = mVal;
                    }
                }
            }
            else
            {
                // Find the destination object
                arTarget = this.FindPathName( sDest, oCaller.cwd );
                if( !arTarget ) return "Not found: " + sDest;
                sDestName = arTarget[1];
                oDest = arTarget[0];

                // Check if the target exists
                if( exists( oDest, sDestName ) )
                {
                    if( !this.AskYesNo( oCaller, "Already exists, overwrite (y/n)? " ) )
                        return 1;
                }

                // Copy the attribute
                oDest[sDestName] = mValue;
            }

            return 1;
        }
    }

    return "Usage: cp <source> <destination>";
}

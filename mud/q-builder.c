[$.lib.cmd.imm.build]
-> $.lib.basic.command
exitDirs = ["n:North", "s:South", "e:East", "w:West", "ne:North East", "nw:North West", "se:South East", "sw:South West", "u:Up", "d:Down", "", "x:Exit to exits menu"]
buildMenu = ["Create new room", "Create new item", "", "x:Exit the builder"]
defaultExitClass = $.mud.class.exit.path
exitClasses = $.mud.class.exit
exitListMenu = ["", "a:Add an exit", "x:Exit to main menu"]
exitMenu = ["Set exit class", "Set destination", "Set leave message", "Set arrive message", "Set player message", "Edit exit actions", "", "d:Delete this exit", "x:Exit to exits menu"]
extrasListMenu = ["", "a:Add an extra", "x:Exit to main menu"]
extrasMenu = ["Set Name", "Set keywords", "Set description", "", "d:Delete this extra", "x:Exit to extras menu"]
invMenu = ["", "a:Add an object", "d:Delete an object", "x:Exit to main menu"]
itemMenu = ["Set item name", "Set item description", "Modify extras", "Edit item notes", "", "x:Exit the item editor"]
newItem = $.lib.basic.item
newRoom = $.lib.basic.room
notesGuide = "Please include your name in [brackets] before your comment. It is considered rude to remove others' comments without their permission. Please be aware that only one person can safely modify the room comments at a time."
roomClasses = $.mud.class.room
roomMenu = ["Set room name", "Set room class", "Set room description", "Set light level", "Modify exits", "Modify room extras", "Modify inventory", "Edit room notes", "", "x:Exit the room editor", "g:Exit and go to the room"]
styleGuide = "Please enter a description for the room beginning with a capital letter and including appropriate punctuation. Ask any admin for more detailed style guidelines."

AddExit( oRoom, sRoomPath, oCaller )
{
    sChoice = this.DisplayMenu( oCaller, "Quintiqua Room Exit: " + sRoomPath, this.exitDirs );
    if (sChoice == "x") return 0;
    sDir = this.validExits[sChoice];
    // Check if the exit already exists
    foreach (arExit in oRoom.exits) {
        if (sDir == arExit[0]) {
            oCaller.Message( "\nAn exit #W" + sDir + "#n already exists." );
            return 0;
        }
    }
    // Add the exit to the room
    oRoom.exits += [ [ sDir, null, this.defaultExitClass, null, null, null ] ];
    oCaller.Message( "\nAdded an exit #W" + sDir + "#n." );
    return this.ModifyExit( oRoom.exits[-1], oRoom, sRoomPath, oCaller );
}

AddExtra( oRoom, sTitle, oCaller )
{
    oCaller.Message( "\nQuintiqua Room Extra\n\n"
      "Please enter a name in lower-case. Do not include any punctuation.\n" );

    sName = trim( oCaller.ReadLine( "New name: " ) );
    if( sName == "" ) return 0;

    if( sName[-1..-1] == "s" ) sPlural = sName + "es";
    else sPlural = sName + "s";

    oRoom.items += [ [ sName + ",", sPlural + ",", "", sName, "" ] ];

    return this.ModifyExtra( length(oRoom.items)-1, oRoom, oCaller );
}

Command( sArgs, oCaller )
{
    if( sArgs == "" )
    {
        if( !oCaller.location ) return "You are in limbo.";
        oThing = oCaller.location;
    }
    else if( sArgs == "." )
    {
        oThing = oCaller.cwd;
    }
    else
    {
        // Find the specified target
        oTarget = this.FindPathName( sArgs, oCaller.cwd );
        if( !oTarget ) return "Not found: " + sArgs;
        sName = oTarget[1];
        oTarget = oTarget[0];

        // Check if the target already exists
        if( !exists( oTarget, sName ) )
        {
            oThing = this.CreateNew( sName, oTarget, oCaller );
            if( !oThing ) return 1;
        }
        else
        {
            // Check if the existing target can be edited
            if( !objectp(oTarget[sName]) ) return "Not an object: " + sArgs;

            oThing = oTarget[sName];
        }
    }

    // Determine object type from its base objects
    arBase = inherit_list( oThing );
    foreach( oBase in arBase )
    {
        if( oBase == this.newRoom ) return this.EditRoom( oThing, oCaller );
        if( oBase == this.newItem ) return this.EditItem( oThing, oCaller );
    }

    return "Cannot edit object: $" + object_path(oThing);
}

CreateNew( sName, oTarget, oCaller )
{
    sPath = "$" + object_path(oTarget) + "." + sName;
    repeat
    {
        nWait = 0;
        sChoice = this.DisplayMenu( oCaller, "Quintiqua Builder: " + sPath, this.buildMenu );
        switch( sChoice )
        {
            case "1":
                // Create a room with this name
                oThing = new object( sName, oTarget );
                add_inherit( oThing, this.newRoom );
                sPath = replace( object_path( oThing ), "$", "$$" );
                oCaller.Message( "Created room: " + sPath );
                return oThing;

            case "2":
                // Create a item with this name
                oThing = new object( sName, oTarget );
                add_inherit( oThing, this.newItem );
                sPath = replace( object_path( oThing ), "$", "$$" );
                oCaller.Message( "Created item: " + sPath );
                return oThing;

            case "x":
                return null;
        }
    }
}

EditDescription( sDesc, sTitle, oCaller )
{
    sTitle = "Quintiqua Description: " + sTitle + "\n\n" + this.styleGuide;
    sNewDesc = this.SimpleEdit( oCaller, sTitle, sDesc, 1 );
    if( sNewDesc == null ) sNewDesc = sDesc;
    return sNewDesc;
}

EditExits( oRoom, sRoomPath, oCaller )
{
    repeat
    {
        arList = [];
        arExits = oRoom.exits;
        foreach( arExit in arExits )
        {
            if( arExit[1] )
                arList += [ arExit[0] + " -> $" + object_path(arExit[1]) ];
            else
                arList += [ arExit[0] + " -> nowhere" ];
        }
        arList += this.exitListMenu;
        sChoice = this.DisplayMenu( oCaller, "Quintiqua Room Exits: " + sRoomPath, arList );
        switch( sChoice )
        {
            case "a":
                this.AddExit( oRoom, sRoomPath, oCaller );
                break;

            case "x":
                return 0;

            default:
                nIndex = to_int(sChoice) - 1;
                this.ModifyExit( arExits[nIndex], oRoom, sRoomPath, oCaller );
                break;
        }
    }
}

EditExtras( oRoom, sRoomPath, oCaller )
{
    repeat
    {
        arList = [];
        arItems = oRoom.items;
        foreach( arInfo in arItems )
        {
            if( length(arInfo[2]) )
                arList += [ arInfo[3] + " [" + arInfo[0][0..-2] + " (" + arInfo[2][0..-2] + ")]" ];
            else
                arList += [ arInfo[3] + " [" + arInfo[0][0..-2] + "]" ];
        }
        arList += this.extrasListMenu;
        sChoice = this.DisplayMenu( oCaller, "Quintiqua Room Extras: " + sRoomPath, arList );
        switch( sChoice )
        {
            case "a":
                this.AddExtra( oRoom, sRoomPath, oCaller );
                break;

            case "x":
                return 0;

            default:
                nIndex = to_int(sChoice) - 1;
                this.ModifyExtra( nIndex, oRoom, oCaller );
                break;
        }
    }
}

EditInventory( oRoom, sRoomPath, oCaller )
{
    repeat
    {
        sChoice = this.DisplayMenu( oCaller, "Quintiqua Room Inventory: " + sRoomPath, this.invMenu );
        switch( sChoice )
        {
            case "x":
                return 0;
        }
    }
}

EditItem( oItem, oCaller )
{
    sItemPath = "$" + object_path(oItem);
    repeat
    {
        nWait = 0;
        sChoice = this.DisplayMenu( oCaller, "Quintiqua Item Editor: " + sItemPath, this.itemMenu );
        switch( sChoice )
        {
            case "1":
                nWait = this.SetName( oItem, sItemPath, oCaller );
                break;

            case "2":
                nWait = this.SetDescription( oItem, sItemPath, oCaller );
                break;

            case "3":
                nWait = this.EditItems( oItem, sItemPath, oCaller );
                break;

            case "4":
                nWait = this.EditNotes( oItem, sItemPath, oCaller );
                break;

            case "x":
                return 1;
        }

        // if( nWait ) oCaller.ReadLine( "Press ENTER to continue... " );
    }
}

EditKeywords( arInfo, sTitle, oCaller )
{
    oCaller.Message( "\nQuintiqua Keywords: " + sTitle + "\n\n"
      "Current nouns: " + arInfo[0][0..-2] +
      "\n      plurals: " + arInfo[1][0..-2] +
      "\n   adjectives: " + arInfo[2][0..-2] +
      "\n\nPlease enter a new list of nouns, plurals and adjectives "
      "seperated by commas. To keep the current list, just press enter. "
      "To clear the list, enter 'none'.\n" );

    sNouns = trim( lower_case( oCaller.ReadLine( "Nouns (enter for no change): " ) ) );
    sPlurals = trim( lower_case( oCaller.ReadLine( "Plurals (enter for no change): " ) ) );
    sAdjs = trim( lower_case( oCaller.ReadLine( "Adjectives (enter for no change): " ) ) );

    if( length(sNouns) )
    {
    if( sNouns == "none" ) sNouns = "";
        sNewNouns = "";
        foreach( sWord in split(sNouns, ",") )
            sNewNouns += trim(sWord) + ",";
        arInfo[0] = sNewNouns;
    }

    if( length(sPlurals) )
    {
    if( sPlurals == "none" ) sPlurals = "";
        sNewPlurals = "";
        foreach( sWord in split(sPlurals, ",") )
            sNewPlurals += trim(sWord) + ",";
        arInfo[1] = sNewPlurals;
    }

    if( length(sAdjs) )
    {
    if( sAdjs == "none" ) sAdjs = "";
        sNewAdjs = "";
        foreach( sWord in split(sAdjs, ",") )
            sNewAdjs += trim(sWord) + ",";
        arInfo[2] = sNewAdjs;
    }

    oCaller.Message( "\nKeywords have been modified.\n\n"
      "Current nouns: " + arInfo[0][0..-2] +
      "\n      plurals: " + arInfo[1][0..-2] +
      "\n   adjectives: " + arInfo[2][0..-2] + "\n\n" );

    return arInfo;
}

[$.lib.cmd.imm.build]
EditMessage( sMsg, oRoom, sTitle, oCaller )
{
    oCaller.Message("NOT IMPLEMENTED - sorry.");
    return 0;
}

EditName( sName, sTitle, oCaller )
{
    oCaller.Message( "\nQuintiqua Name: " + sTitle + "\n\n"
      "Current name: #W" + sName + "#n\n\n"
      "Please enter a new name. If the name is a proper noun, begin "
      "with a capital letter. Do not include any punctuation. To keep the "
      "existing name, just press enter.\n" );

    sNewName = trim( oCaller.ReadLine( "New name (press enter for no change): " ) );
    if( sNewName == "" ) return sName;

    oCaller.Message( "\nName set to: #W" + sNewName + "#n" );
    return sNewName;
}

EditNotes( oRoom, sRoomPath, oCaller )
{
    sTitle = "Quintiqua Room Notes: " + sRoomPath + "\n\n" + this.notesGuide;
    sComments = this.SimpleEdit( oCaller, sTitle, oRoom.comments );
    if( sComments == null ) return 0;

    // Modify the room comments
    if( length(sComments) < 1 ) oRoom.comments = null;
    else oRoom.comments = sComments;

    oCaller.Message( "\nRoom comments set." );
    return 1;
}

EditRoom( oRoom, oCaller )
{
    sRoomPath = "$" + object_path(oRoom);
    repeat
    {
        nWait = 0;
        sChoice = this.DisplayMenu( oCaller, "Quintiqua Room Editor: " + sRoomPath, this.roomMenu );
        switch( sChoice )
        {
            case "1":
                nWait = this.SetName( oRoom, sRoomPath, oCaller );
                break;

            case "2":
                nWait = this.SetClass( oRoom, sRoomPath, oCaller );
                break;

            case "3":
                nWait = this.SetDescription( oRoom, sRoomPath, oCaller );
                break;

            case "4":
                nWait = this.SetLight( oRoom, sRoomPath, oCaller );
                break;

            case "5":
                nWait = this.EditExits( oRoom, sRoomPath, oCaller );
                break;

            case "6":
                nWait = this.EditExtras( oRoom, sRoomPath, oCaller );
                break;

            case "7":
                nWait = this.EditInventory( oRoom, sRoomPath, oCaller );
                break;


            case "8":
                nWait = this.EditNotes( oRoom, sRoomPath, oCaller );
                break;

            case "x":
                return 1;

            case "g":
                return oCaller.Move( oRoom, oCaller.TheName() + " teleports out.",
                  oCaller.AName() + " teleports in." );
        }

        // if( nWait ) oCaller.ReadLine( "Press ENTER to continue... " );
    }
}

ModifyExit( arExit, oRoom, sRoomPath, oCaller )
{
    while( length(arExit) < 5 ) arExit += [ null ];

    repeat
    {
        sDir = arExit[0];
        if( arExit[1] ) sDest = "$" + object_path( arExit[1] ); else sDest = "nowhere";
        sTitle = sDir + " -> " + sDest;

        sChoice = this.DisplayMenu( oCaller, "Quintiqua Room Exit: " + sTitle, this.exitMenu );
        switch( sChoice )
        {
            case "1":
                this.SetExitClass( arExit, oRoom, sTitle, oCaller );
                break;

            case "2":
                this.SetExitDest( arExit, oRoom, sTitle, oCaller );
                break;

            case "3":
                if( arExit[3] ) sMsg = arExit[3]; else sMsg = "";
                sLeave = this.EditMessage( sMsg, oRoom, sTitle, oCaller );
                arExit[3] = sMsg;
                break;

            case "4":
                if( arExit[4] ) sMsg = arExit[4]; else sMsg = "";
                sLeave = this.EditMessage( sMsg, oRoom, sTitle, oCaller );
                arExit[4] = sMsg;
                break;

            case "5":
                if( arExit[5] ) sMsg = arExit[5]; else sMsg = "";
                sLeave = this.EditMessage( sMsg, oRoom, sTitle, oCaller );
                arExit[5] = sMsg;
                break;

            case "d":
                if( this.AskYesNo( oCaller, "Are you sure you wish to delete this exit (y/n)? " ) )
                {
                    foreach( arExit in oRoom.exits )
                    {
                        if( arExit[0] == sDir )
                        {
                            oRoom.exits -= [ arExit ];
                            oCaller.Message( "\nDeleted the exit #W" + sDir + "#n." );
                            return 1;
                        }
                    }
                    oCaller.Message( "\nCould not find the exit #W" + sDir + "#n to delete." );
                    return 0;
                }
                break;

            case "x":
                return 0;
        }
    }
}

ModifyExtra( nIndex, oThing, oCaller )
{
    arInfo = oThing.items[nIndex];
    while( length(arInfo) < 5 ) arInfo += [ "" ];

    repeat
    {
        if( length(arInfo[2]) )
            sTitle = arInfo[3] + " [" + arInfo[0][0..-2] + " (" + arInfo[2][0..-2] + ")]";
        else
            sTitle = arInfo[3] + " [" + arInfo[0][0..-2] + "]";

        sChoice = this.DisplayMenu( oCaller, "Quintiqua Extra: " + sTitle, this.extrasMenu );
        switch( sChoice )
        {
            case "1":
                arInfo[3] = this.EditName( arInfo[3], sTitle, oCaller );
                break;

            case "2":
                arInfo = this.EditKeywords( arInfo, sTitle, oCaller );
                break;

            case "3":
                arInfo[4] = this.EditDescription( arInfo[4], sTitle, oCaller );
                break;

            case "d":
                if( this.AskYesNo( oCaller, "Are you sure you wish to delete this extra (y/n)? " ) )
                {
                    if( nIndex < 1 ) oThing.items = oThing.items[1..];
                    else oThing.items = oThing.items[0..nIndex-1] + oThing.items[nIndex+1..];

                    oCaller.Message( "\nDeleted extra: " + sTitle );
                    return 1;
                }
                break;

            case "x":
                oThing.items[nIndex] = arInfo;
                return 0;
        }
    }
}

SetClass( oRoom, sRoomPath, oCaller )
{
    arClasses = keys( this.roomClasses ) + [ "", "x:Exit to main menu" ];
    if( oRoom.class ) sCurrent = to_string( this.MemberArray( arClasses, oRoom.class.name ) + 1 );
    else sCurrent = "";
    sChoice = this.DisplayMenu( oCaller, "Quintiqua Room Class: " + sRoomPath, arClasses, null, sCurrent );
    switch( sChoice )
    {
        case "x":
            return 0;

        default:
            sClass = arClasses[to_int(sChoice) - 1];
            oRoom.class = this.roomClasses[sClass];
            oCaller.Message( "Room class set to: #W" + oRoom.class.name + "#n" );
            return 1;
    }
}

SetDescription( oRoom, sRoomPath, oCaller )
{
    sTitle = "Quintiqua Room Description: " + sRoomPath + "\n\n" + this.styleGuide;
    bDesc2 = exists( oRoom, "desc2" );
    if( bDesc2 ) sDesc = oRoom.desc2; else sDesc = oRoom.desc;
    sDesc = this.SimpleEdit( oCaller, sTitle, sDesc, 1 );
    if( sDesc == null ) return 0;

    // Modify the room description
    if( bDesc2 ) oRoom.desc2 = sDesc; else oRoom.desc = sDesc;
    oCaller.Message( "\nRoom description set." );
    return 1;
}

SetExitClass( arExit, oRoom, sTitle, oCaller )
{
    arClasses = keys( this.exitClasses ) + [ "", "x:Exit to main menu" ];
    if( arExit[2] ) sCurrent = to_string( this.MemberArray( arClasses, arExit[2].name ) + 1 );
    else sCurrent = "";
    sChoice = this.DisplayMenu( oCaller, "Quintiqua Exit Class: " + sTitle, arClasses, null, sCurrent );
    switch( sChoice )
    {
        case "x":
            return 0;

        default:
            sClass = arClasses[to_int(sChoice) - 1];
            arExit[2] = this.exitClasses[sClass];
            oCaller.Message( "Exit class set to: #W" + arExit[2].name + "#n" );
            return 1;
    }
}

SetExitDest( arExit, oRoom, sTitle, oCaller )
{
    if( arExit[1] ) sDest = "$" + object_path( arExit[1] ); else sDest = "nowhere";
    oCaller.Message( "\nQuintiqua Exit Destination: " + sTitle + "\n\n"
      "Current destination: #W" + sDest + "#n\n\n"
      "Please enter the room path of the new destination room relative to "
      "this room, or an absolute room path beginning with $$. "
      "To keep the existing destination room, just press enter.\n" );

    repeat
    {
        sPath = trim( oCaller.ReadLine( "New destination (press enter for no change): " ) );
        if( sPath == "" ) return 0;

        // Check if the destination exists
        oDest = find_object( sPath, parent(oRoom) );
        if( !objectp(oDest) )
        {
            oCaller.Message( "\nThe room #W" + sPath + "#n was not found." );
            continue;
        }

        sPath = object_path( oDest );

        // Check if the destination is a room
        if( !functionp(oDest.AddObject) )
        {
            oCaller.Message( "\nThe object #W" + sPath + "#n is not a room." );
            continue;
        }

        break;
    }

    // Modify the destination
    arExit[1] = oDest;
    oCaller.Message( "\nExit destination set to: #W" + sPath + "#n" );
    return 1;
}

SetLight( oRoom, sRoomPath, oCaller )
{
    if( !oRoom.class )
    {
        oCaller.Message( "\n#WThis room has no room class. You must set a room "
            "class before selecting a lighting level.#n\n" );
        return 1;
    }

    arLevels = oRoom.class.lightOptions + [ "", "x:Exit to main menu" ];
    sCurrent = to_string( oRoom.light + 1 );
    sChoice = this.DisplayMenu( oCaller, "Quintiqua Room Lighting: " + sRoomPath, arLevels, null, sCurrent );
    switch( sChoice )
    {
        case "x":
            return 0;

        default:
            oRoom.light = nLevel = to_int(sChoice) - 1;
            oCaller.Message( "Room lighting set to: #W" + oRoom.class.lightOptions[nLevel] + "#n" );
            return 1;
    }
}

SetName( oRoom, sRoomPath, oCaller )
{
    oCaller.Message( "\nQuintiqua Room Name: " + sRoomPath + "\n\n"
      "Current name: #W" + oRoom.name + "#n\n\n"
      "Please enter a new name for this room, beginning with a capital "
      "letter, but excluding any trailing punctuation. To keep the "
      "existing room name, just press enter.\n" );

    sName = trim( oCaller.ReadLine( "New room name (press enter for no change): " ) );
    if( sName == "" ) return 0;

    // Modify the room name
    oRoom.name = sName;
    oCaller.Message( "\nRoom name set to: #W" + oRoom.name + "#n" );
    return 1;
}

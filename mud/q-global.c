[$.globals]
allowedExits = {"down" : 1, "east" : 1, "north" : 1, "northeast" : 1, "northwest" : 1, "south" : 1, "southeast" : 1, "southwest" : 1, "up" : 1, "west" : 1}
combatDaemon = $.lib.daemon.combat
loginDaemon = $.lib.daemon.login
worldSys = $.lib.daemon.worldsys
mudName = "Quintiqua"
mudVersion = "3.0"
npcDaemon = $.lib.daemon.npcact
[$.globals]
validExits = {"u" : "up", "d" : "down", "n" : "north", "s" : "south", "e" : "east", "w" : "west", "nw" : "northwest", "ne" : "northeast", "sw" : "southwest", "se" : "southeast", "up" : "up", "down" : "down", "north" : "north", "south" : "south", "east" : "east", "west" : "west", "northwest" : "northwest", "northeast" : "northeast", "southwest" : "southwest", "southeast" : "southeast" }
oppositeDir = {"down" : "up", "east" : "west", "north" : "south", "northeast" : "southwest", "northwest" : "southeast", "south" : "north", "southeast" : "northwest", "southwest" : "northeast", "up" : "down", "west" : "east"}
oppositeExit = {"down" : "above", "east" : "the west", "north" : "the south", "northeast" : "the southwest", "northwest" : "the southeast", "south" : "the north", "southeast" : "the northwest", "southwest" : "the northeast", "up" : "below", "west" : "the east"}
definiteExit = {"down" : "below", "east" : "the east", "north" : "the north", "northeast" : "the northeast", "northwest" : "the northwest", "south" : "the south", "southeast" : "the southeast", "southwest" : "the southwest", "up" : "above", "west" : "the west"}
terminalDaemon = $.lib.daemon.terminal

[$.globals]
oppositeExitID = {"d":"u", "u":"d", "e":"w", "w":"e", "n":"s", "s":"n", "ne":"sw", "sw":"ne", "nw":"se", "se":"nw"}
exitNameToID = {"down":"d", "up":"u", "east":"e", "west":"w", "north":"n", "south":"s", "northeast":"ne", "northwest":"nw", "southeast":"se", "southwest":"sw"}


[$.globals]
invalidIdMsg = "ID must contain only lower-case letters/numbers/dash";
ValidId(id) {
  if (!id) return 0;
  return (skip(id,0,"abcdefghijklmnopqrstuvwxyz0123456789-") < 0);
}

[$.globals]
RoomId(room) {
  if (clonep(room)) room = parent(room);
  id = object_path(room);
  if (id[0..6]=="$.area.") return id[7..];
  if (id[0..6]=="$.home.") return "@"+id[7..];
  return "?"+id[2..];
}

[$.globals]
FindRoomById(id) {
  // this will do partial matching soon.
  return this.RoomForId(id);
}

[$.globals]
RoomForId(id) {
  if (id[0]=="@") obj = find_object(id[1..], $.home);
  else obj = find_object(id, $.area);
  if (obj.isRoom) return obj;
  return null;
}

ItemId(item) {
  if (clonep(item)) item = parent(item);
  id = object_path(room);
  if (id[0..7]=="$.items.") return id[8..];
  if (id[0..6]=="$.home.") return "@"+id[7..];
  return "?"+id[2..];
}

MobId(mob) {
  if (clonep(mob)) mob = parent(mob);
  id = object_path(room);
  if (id[0..6]=="$.mobs.") return id[7..];
  if (id[0..6]=="$.home.") return "@"+id[7..];
  return "?"+id[2..];
}

ItemForId(id) {
  if (id[0]=="@") obj = find_object($.home, id[1..]);
  else obj = find_object($.items, id);
  if (exists(obj,"isItem") && obj.isItem) return obj;
  return null;
}

MobForId(id) {
  if (id[0]=="@") obj = find_object($.home, id[1..]);
  else obj = find_object($.mobs, id);
  if (exists(obj,"isMob") && obj.isMob) return obj;
  return null;
}



AskYesNo( oUser, sPrompt, sMessage )
{
    if( sMessage ) oUser.Message( sMessage );
    if( !sPrompt ) sPrompt = "Do you wish to continue (y/n)? ";

    repeat
    {
        sConfirm = trim( oUser.RequestLine( sPrompt ) );
        if( sConfirm == "y" ) return 1;
        if( sConfirm == "n" ) return 0;
    }
}

AssignChar( sStr, nIndex, sChar )
{
    if( nIndex ) return sStr[0..nIndex-1] + sChar + sStr[nIndex+1..];
    return sChar + sStr[1..];
}

Center( sText, nCols )
{
    nLen = length(sText);
    if( nLen >= nCols ) return sText[0..nCols - 3] + "..";
    colCount = (nCols / 2) - (nLen / 2);
    lPad = this.RepeatString( " ", colCount );
    if(length(sText) % 2 == 0) {
        rPad = lPad;
    } else {
        rPad = this.RepeatString( " ", colCount -1 );
    }

    return lPad + sText + rPad;
}

DescribeError( err, bUncaught )
{
    // err = [ number, description, call-stack[ fnCall, nLine, ... ] ]

    sResult = "--- \n";

    try
    {
        // Build a nice runtime error description and call stack
        if( bUncaught ) sResult += "uncaught runtime error:\n";
        sResult += err[1] + " (" + err[0] + ")\n";
        arStack = err[2];
        i = 0;
        nLen = sizeof(arStack);
        while( i < nLen )
        {
            fnCall = arStack[i];
            nLine = arStack[i + 1];
            i += 2;
            sPath = object_path( function_context(fnCall) );
            if( !sPath ) sPath = "(object)";
            sResult += function_name( fnCall ) + " at line " + nLine + ", in " + sPath + "\n";
        }

        return sResult;
    }
    catch( ierr )
    {
    }

    try
    {
        // Don't Panic. Just add any info we can.
        sResult += "[Eek! Error in DescribeError]\n";
        sResult += "description: " + ierr[1] + "\n";
        sResult += "number: " + ierr[0] + "\n";
        sResult += "line: " + ierr[2][1] + "\n";
        sResult += "[Error was: " + ierr[1] + " (" + ierr[0] + ") at line " + ierr[2][1] + "]\n";
    }

    return sResult;
}

DisplayMenu( oUser, sTitle, arOptions, sPrompt, sCurrent )
{
    if( !stringp(sPrompt) ) sPrompt = "Please enter your choice: ";

    // Build the menu string and options map
    if( sTitle ) sMenu = "\n" + sTitle + "\n\n";
    else sMenu = "\n";
    mpOptions = new mapping;
    nOption = 1;
    foreach( sOption in arOptions )
    {
        // Check for seperator items
        if( sOption == "" )
        {
            sMenu += "\n";
            continue;
        }

        // Split around the delimiter (colon) if present
        nOfs = search( sOption, ":" );
        if( nOfs > -1 )
        {
            sCmd = lower_case( sOption[0..nOfs-1] );
            sOption = sOption[nOfs+1..];
        }
        else
        {
            sCmd = to_string( nOption );
            nOption += 1;
        }

        // Add the option to the menu
        if( sCmd == sCurrent ) sMenu += "#W> ";
        else sMenu += "  ";
        sMenu += sCmd + ". " + sOption + "#n\n";
        mpOptions[sCmd] = 1;
    }

    // Display the menu
    oUser.Message( sMenu );

    repeat
    {
        // Read and validate the user's choice
        sChoice = lower_case( trim( oUser.ReadLine( sPrompt ) ) );
        if( exists( mpOptions, sChoice ) )
        {
            oUser.Message( "" );
            return sChoice;
        }
        if( length( sChoice ) )
        {
            // Display an error
            oUser.Message( "\nPlease select one of the options from the menu." );
        }
        else
        {
            // Display the menu
            oUser.Message( sMenu );
        }
    }
}

[$.globals]
FindItemID(id, me) {
  items = $.mud.items;
  if (exists(items, id)) return items[id];
  if (me.creator && id[0] == "@") {
    item = find_object($.home, id[1..]);
    if (!item) return "No matching item ID: "+id;
    return item;
  }
  matches = [];
  foreach(name in items) {
    if (search(name, id) >= 0) {
      matches += [items[name]];
    }
  }
  if (!matches) return "No matching item ID: "+id;
  if (length(matches) > 1) return "More than one matching ID: " + join(matches, ", ");
  return matches[0];
}

FindPartial(obj, name) {
    full = this.ExistsPartial(obj, name);
    if (full) return obj[full];
    return null;
}

ExistsPartial( oContext, sName )
{
    // Check for an exact match
    if( length( sName ) < 1 ) return null;
    if( exists( oContext, sName ) ) return sName;

    sName = lower_case( sName );
    nLen = length( sName ) - 1;

    // Look for a partial match
    sMatch = null;
    foreach( sAttrib in oContext )
    {
        if( lower_case(sAttrib[0..nLen]) == sName )
        {
            // Find the first match alphabetically
            if( !sMatch ) sMatch = sAttrib;
            else if( lower_case(sAttrib) < lower_case(sMatch) ) sMatch = sAttrib;
        }
    }

    // Look in inherited objects
    foreach( oParent in inherit_list(oContext) )
    {
        sAttrib = this.ExistsPartial( oParent, sName );
        if( !sAttrib ) continue;

        // Find the first match alphabetically
        if( !sMatch ) sMatch = sAttrib;
        else if( lower_case(sAttrib) < lower_case(sMatch) ) sMatch = sAttrib;
    }

    return sMatch;
}

FindPathName( sPath, oContext )
{
    // Find the last dot in the path
    nLast = rsearch( sPath, "." );
    if( nLast > -1 )
    {
        // Find the target object
        oResult = find_object( sPath[..nLast - 1], oContext );
        if( !oResult ) return null;

        return [ oResult, sPath[nLast + 1..] ];
    }
    else
    {
        return [ oContext, sPath ];
    }
}

FindPathValue( sPath, oContext )
{
    oResult = find_object( sPath );
    if( oResult != null ) return oResult;

    // Find the last dot in the path
    nLast = rsearch( sPath, "." );
    if( nLast > -1 )
    {
        // Find the target object
        oResult = find_object( sPath[..nLast - 1], oContext );
        if( !oResult ) return null;

        return oResult[sPath[nLast + 1..]];
    }

    return oContext[sPath];
}

LengthCC( sText )
{
    nLen = length(sText);
    nOfs = 0;
    sPrefix = "#";
    sEmpty = "";
    repeat {
        nOfs = search(sText, sPrefix, nOfs);
        if (nOfs < 0) break; // finished
        sCode = sText[nOfs+1];
        if (sCode == sPrefix || sCode == sEmpty) nLen -= 1; // escaped # or trailing #
        else nLen -= 2; // account for colour code
        nOfs += 2; // skip the color sequence
    }
    return nLen;
}

ListItems( arItems, oAgent )
{
    if( !arrayp(arItems) ) arItems = [arItems];

    //arItems -= [null];
    //arCopy = arItems - [oAgent];
    arCopy = arItems;
    nRemain = nSize = sizeof(arCopy);
    if( nSize )
    {
        sResult = "";
        foreach( oThing in arCopy )
        {
            // Get the item's name for this observer
            if (oThing == oAgent) sName = "you";
            else sName = oThing.Name( oAgent );
            if( search( "aeiou", sName[0..0] ) != -1 ) sName = "an " + sName;
            else if( search( "bcdfghjklmnpqrstvwxyz", sName[0..0] ) != -1 ) sName = "a " + sName;
            sResult += sName;
            nRemain -= 1;
            if( nRemain > 1 ) sResult += ", ";
            else if( nRemain == 1 ) sResult += " and ";
        }
        return sResult;
    }

    return "";
}

[$.globals]
MergePatterns(under, over, width, uScroll, oScroll) {
    if (!uScroll || uScroll < 0) uScroll = 0;
    if (!oScroll || oScroll < 0) oScroll = 0;
    uCol = oCol = rCol = "n";
    uPos = oPos = 0;
    s = "";
    while (width) {
        // make the patterns advance.
        uScroll += 1;
        oScroll += 1;
        // advance both patterns.
        while (uScroll > 0) {
            uChar = under[uPos]; uPos += 1;
            if (!uChar) { uPos = 0; continue; } // wrap around.
            else if (uChar != "#") { uScroll -= 1; continue; } // letter or space.
            uChar = under[uPos]; uPos += 1;
            if (uChar == "#") { uScroll -= 1; continue; } // escaped #.
            if (uChar) uCol = uChar; // colour.
        }
        while (oScroll > 0) {
            oChar = over[oPos]; oPos += 1;
            if (!oChar) { oPos = 0; continue; } // wrap around.
            else if (oChar != "#") { oScroll -= 1; continue; } // letter or space.
            oChar = over[oPos]; oPos += 1;
            if (oChar == "#") { oScroll -= 1; continue; } // escaped #.
            if (oChar) oCol = oChar; // colour.
        }
        // emit an output character.
        if (oChar != " ") {
            if (rCol != oCol) { rCol = oCol; s += "#"+rCol; }
            s += oChar;
        } else {
            if (rCol != uCol) { rCol = uCol; s += "#"+uCol; }
            s += uChar;
        }
        width -= 1;
    }
    return s;
}

ListMultipleShort( arItems, oAgent )
{
    if( !arrayp(arItems) ) arItems = [arItems];

    //arItems -= [null];
    //arCopy = arItems - [oAgent];
    arCopy = arItems;
    nRemain = nSize = sizeof(arCopy);
    if( nSize )
    {
        sResult = "";
        foreach( oThing in arCopy )
        {
            // Get the item's short description for this observer
            sResult += oThing.Short( oAgent );
            nRemain -= 1;
            if( nRemain > 1 ) sResult += ", ";
            else if( nRemain == 1 ) sResult += " and ";
        }
        if( nSize > 1 ) return sResult + " are here.";
        else return sResult + " is here.";
    }

    return "";
}

MemberArray( arList, mValue )
{
    nIndex = 0;
    foreach( mEntry in arList )
    {
        if( mEntry == mValue ) return nIndex;
        nIndex += 1;
    }
    return -1;
}

[$.globals]
SplitClean(text) {
    words = split(lower_case(trim(replace(replace(text,","," "),";"," "))), " ");
    keep = [];
    foreach(word in words) {
        word = trim(word);
        if (word) keep += [word];
    }
    return keep;
}

CleanWords(words, i) {
    if (!i) i = 0;
    len = length(words);
    keep = [];
    while (i<len) {
        word = lower_case(trim(replace(replace(words[i],",",""),";","")));
        if (word) keep += [word];
        i += 1;
    }
    return keep;
}

[$.globals]
PadCC(text, width) {
  // HACK could be done better with rsearch and spans.
  // But this is going to be replaced with #[...#] for word-wrap.
  len = this.LengthCC(text);
  if (len > width) {
    while (len > width) {
      // HACK: does not count escaped ##.
      if (text[-1] != "#") len -= 1;
      text = text[0..-2];
    }
    return text;
  }
  if (len < width) {
    while (len < width) {
      text += " ";
      len += 1;
    }
  }
  return text;
}

PadString( sText, nCols )
{
    nLen = length(sText);
    if( nLen >= nCols ) return sText[0..nCols - 1];
    return sText + this.RepeatString( " ", nCols - nLen );
}

RepeatString( sString, nRepeat )
{
    sResult = "";
    while( nRepeat > 0 )
    {
        sResult += sString;
        nRepeat -= 1;
    }
    return sResult;
}

ShowEditBuffer( oUser, mText, bRequire )
{
    // Display the current contents of the buffer
    if( length(mText) )
    {
        nLine = 1;
        sBuffer = "\n";
        foreach( sLine in mText )
        {
            sNum = to_string(nLine);
            sBuffer += this.RepeatString( " ", 2 - length(sNum) ) + "#W" + sNum + ".#n " + sLine + "\n";
            nLine += 2;
        }
        oUser.MessageNoWrap( sBuffer + "\n" );
    }
    else
    {
        if( bRequire ) oUser.RawMessage( "\n" );
        oUser.RawMessage( "There is no text in the buffer.\n\n" );
    }
}

EditText( oUser, sTitle, sText, bWrap, undoMsg )
{
    if (length(sText)) {
        if (bWrap) mText = this.terminalDaemon.Wrap(sText, oUser.cols - 4);
        mText = split(sText, "\n");
    } else {
        mText = [];
    }

    bDirty = 0;
    bForce = 0;

    repeat
    {
        // Display the title and instructions
        oUser.Message( "\n" + sTitle );

        if (length(mText) || bForce) {

            // Display the buffer contents
            if (length(mText)) {
                nLine = 1;
                sBuffer = "\n";
                foreach (sLine in mText) {
                    sNum = to_string(nLine);
                    sBuffer += "  "[length(sNum)..] + "#W" + sNum + ".#n " + sLine + "\n";
                    nLine += 2;
                }
                oUser.MessageNoWrap( sBuffer + "\n" );
            } else {
                oUser.MessageNoWrap( "No lines entered.\n" );
            }

            repeat {
                oUser.Message(
                  "Enter a line number to insert or replace, or a range to replace."
                  "\nEnter '#ox#n' to finish, '#oq#n' to discard changes, '#oall#n' to replace all.\n\n" );

                sLine = trim(oUser.ReadLine("[##, ##-##, a, b, all, x, q] "));

                // Check for a stop command.
                if (sLine == "x") {
                    msg = "\nKeeping changes. ";
                    if (undoMsg) msg += undoMsg;
                    oUser.Message(msg);

                    if (bWrap) return join(mText, " "); // single line
                    else return join(mText, "\n"); // multi-line
                }

                if (sLine == "q") {
                    msg = "\nDiscarding all changes. ";
                    if (undoMsg) msg += undoMsg;
                    oUser.Message(msg);
                    return null;
                }

                if (sLine == "all") {
                    oUser.Message("\nReplacing all lines.\n");
                    mText = [];
                    bDirty = 1;
                    insert = 0;
                    apparent = 1;
                    break;
                }

                if (sLine == "a") {
                    insert = length(mText);
                    apparent = length(mText)*2+1;
                    break;
                }
                if (sLine == "b") {
                    insert = 0;
                    apparent = 1;
                    break;
                }

                // Check for a line range.
                ofs = search(sLine, "-");
                if (ofs > 0) {
                    first = to_int(trim(sLine[0..ofs-1]));
                    last = to_int(trim(sLine[ofs+1..]));
                    if (numberp(first) && numberp(last) && first >= 1 && first % 2 == 1 && last >= first && last % 2 == 1) {
                        apparent = first;
                        oUser.Message("\nRemoved lines "+first+" to "+last+".\n");
                        first = to_int(first / 2);
                        last = to_int(last / 2);
                        // Remove the lines in that range, inclusive.
                        if (first == 0) mText = mText[last+1..];
                        else mText = mText[0..first-1] + mText[last+1..];
                        bDirty = 1;
                        insert = first;
                        break; // start editing.
                    }
                    oUser.Message("\nNot a valid range to replace.\n");
                } else {
                    first = to_int(sLine);
                    if (numberp(first) && first >= 0) {
                        apparent = first;
                        insert = to_int(first / 2);
                        if (first % 2 == 1) {
                            // Remove the line before entering insert mode.
                            oUser.Message("\nRemoved line "+apparent+".\n");
                            if (insert == 0) mText = mText[1..];
                            else mText = mText[0..insert-1] + mText[insert+1..];
                            bDirty = 1;
                        }
                        break; // start editing.
                    }
                    oUser.Message("\nNot a valid line number.\n");
                }
            }

        } else {
            // Start entering text.
            apparent = 1;
            insert = 0;
        }

        // Keep displayed numbers consecutive.
        if (apparent > length(mText)*2+1) apparent = length(mText)*2+1;

        oUser.Message("\nInserting lines at line "+apparent+"."
                      "\nEnter '.' to finish inserting.\n\n");
        repeat {
            sNum = to_string(apparent);
            sPrompt = "  "[length(sNum)..] + sNum + ". ";

            sLine = oUser.ReadLine(sPrompt);
            if (sLine == ".") {
                bForce = 1;
                break;
            }
            // Insert the line
            if (insert == 0) mText = [ sLine ] + mText;
            else if (insert >= length(mText)) mText += [ sLine ];
            else mText = mText[0..insert-1] + [ sLine ] + mText[insert..];
            apparent += 1;
            insert += 1;
            bDirty = 1;
        }
    }
}

[$.globals]
EditText2( oUser, sTitle, sText, bWrap, undoMsg )
{
    if (length(sText)) {
        if (bWrap) mText = this.terminalDaemon.Wrap(sText, oUser.cols - 4);
        mText = split(sText, "\n");
    } else {
        mText = [];
    }

    bDirty = 0;
    bForce = 0;

    repeat
    {
        // Display the title and instructions
        oUser.Message( "\n" + sTitle );

        if (length(mText) || bForce) {

            // Display the buffer contents
            if (length(mText)) {
                nLine = 1;
                sBuffer = "\n";
                foreach (sLine in mText) {
                    sNum = to_string(nLine);
                    sBuffer += "  "[length(sNum)..] + "#W" + sNum + ".#n " + sLine + "\n";
                    nLine += 1;
                }
                oUser.MessageNoWrap( sBuffer + "\n" );
            } else {
                oUser.MessageNoWrap( "No lines entered.\n" );
            }

            repeat {
                oUser.Message(
                  "Enter a number to replace, 'a' or 'b' to insert after/before, 'd' to delete."
                  "\nEnter '#ox#n' to finish, '#oq#n' to discard changes, '#oall#n' to replace all.\n\n" );

                sLine = trim(oUser.ReadLine("[##, a ##, b ##, x, q] "));

                // Check for a stop command.
                if (sLine == "x") {
                    msg = "\nKeeping changes. ";
                    if (undoMsg) msg += undoMsg;
                    oUser.Message(msg);

                    if (bWrap) return join(mText, " "); // single line
                    else return join(mText, "\n"); // multi-line
                }

                if (sLine == "q") {
                    msg = "\nDiscarding all changes. ";
                    if (undoMsg) msg += undoMsg;
                    oUser.Message(msg);
                    return null;
                }

                if (sLine == "all") {
                    oUser.Message("\nReplacing all lines.\n");
                    mText = [];
                    bDirty = 1;
                    insert = 0;
                    apparent = 1;
                    break;
                }

                if (sLine == "a") {
                    insert = length(mText);
                    apparent = length(mText)*2+1;
                    break;
                }
                if (sLine == "b") {
                    insert = 0;
                    apparent = 1;
                    break;
                }

                // Check for a line range.
                ofs = search(sLine, "-");
                if (ofs > 0) {
                    first = to_int(trim(sLine[0..ofs-1]));
                    last = to_int(trim(sLine[ofs+1..]));
                    if (numberp(first) && numberp(last) && first >= 1 && first % 2 == 1 && last >= first && last % 2 == 1) {
                        apparent = first;
                        oUser.Message("\nRemoved lines "+first+" to "+last+".\n");
                        first = to_int(first / 2);
                        last = to_int(last / 2);
                        // Remove the lines in that range, inclusive.
                        if (first == 0) mText = mText[last+1..];
                        else mText = mText[0..first-1] + mText[last+1..];
                        bDirty = 1;
                        insert = first;
                        break; // start editing.
                    }
                    oUser.Message("\nNot a valid range to replace.\n");
                } else {
                    first = to_int(sLine);
                    if (numberp(first) && first >= 0) {
                        apparent = first;
                        insert = to_int(first / 2);
                        if (first % 2 == 1) {
                            // Remove the line before entering insert mode.
                            oUser.Message("\nRemoved line "+apparent+".\n");
                            if (insert == 0) mText = mText[1..];
                            else mText = mText[0..insert-1] + mText[insert+1..];
                            bDirty = 1;
                        }
                        break; // start editing.
                    }
                    oUser.Message("\nNot a valid line number.\n");
                }
            }

        } else {
            // Start entering text.
            apparent = 1;
            insert = 0;
        }

        // Keep displayed numbers consecutive.
        if (apparent > length(mText)*2+1) apparent = length(mText)*2+1;

        oUser.Message("\nInserting lines at line "+apparent+"."
                      "\nEnter '.' to finish inserting.\n\n");
        repeat {
            sNum = to_string(apparent);
            sPrompt = "  "[length(sNum)..] + sNum + ". ";

            sLine = oUser.ReadLine(sPrompt);
            if (sLine == ".") {
                bForce = 1;
                break;
            }
            // Insert the line
            if (insert == 0) mText = [ sLine ] + mText;
            else if (insert >= length(mText)) mText += [ sLine ];
            else mText = mText[0..insert-1] + [ sLine ] + mText[insert..];
            apparent += 1;
            insert += 1;
            bDirty = 1;
        }
    }
}

Split( sText, sDelim )
{
    if(!stringp(sDelim)) sDelim = " ";

    // Split around the delimiter if present
    nOfs = search( sText, sDelim );
    if( nOfs > -1 )
    {
        return [ sText[0..nOfs-1], sText[nOfs+1..] ];
    }
    else
    {
        return [ sText ];
    }
}

Table( user, columns, values, colDelim, padding )
{
    userCols = user.cols;
    if( !stringp(colDelim) ) colDelim = "|";

    // if array lengths dont match, error message returns
    if( length(columns) != length(values[0]) ) {
        return "Error: Columns & values must be equal in length.";

    } else {
        output = ""; //A string to hold all of our output
        foreach(row in values) {
            if(!row) continue; // ignore left over nulls
            line = colDelim;
            for(i = 0; i < length(columns); i = i+1 ) {
                pad = this.RepeatString(" ", padding);
                line += pad;
                text = row[i];
                if(!text) continue; // ignore left over nulls
                line += this.Center(text, columns[i]);
                line += pad;
                line += colDelim;
            }
            output += line + "\n";
        }
        return output;
    }
}

[$.globals]
Wrap(text, cols, asList, pad)
{
  try { return this.WordWrap(text, cols, asList, pad); }
  if (asList) return [text];
  return text;
}

[$.globals]
WordWrap( sText, wrapCol, asList, bPad )
{
    if (asList) arResult = [];

    sResult = "";
    srcPos = 0;

    // List of line lengths: wrap each line with a different length,
    // but latch the final length when the list runs out.
    colsList = null;
    if (arrayp(wrapCol)) {
        colsList = wrapCol;
        lineIdx = 0;
        wrapCol = colsList[0];
    }

    while (srcPos < length(sText)) {
        // Find the next newline sequence
        nextCR = search(sText, "\n", srcPos);
        if (nextCR < 0) nextCR = length(sText);
        numVis = 0;

        // Word-wrap a single line of the text
        repeat {
            // Find the next colour prefix symbol
            nextPre = search(sText, "#", srcPos);
            if (nextPre > nextCR || nextPre < 0) nextPre = nextCR;
            spanLen = nextPre - srcPos;

            // Check if this span will overrun the wrap column or fit exactly.
            overrun = numVis + spanLen - wrapCol;
            if (overrun >= 0) {
                // Wrap here: find the src pos to chop at.
                cutPos = nextPre - overrun;
                padPos = cutPos;

                // Go back until we find a space, then
                // go back until we reach a non-space.
                while (cutPos > srcPos && sText[cutPos] != " ") cutPos -= 1;
                while (cutPos > srcPos && sText[cutPos] == " ") cutPos -= 1;

                // If we went back to the start, it means the line does not
                // contain any spaces. Just break at the original cutPos.
                if (cutPos == srcPos) cutPos = padPos;

                // Keep the span up to the end of the word.
                sResult += sText[srcPos..cutPos];

                // Emit a line.
                if (bPad) {
                    // Pad with spaces back up to wrapCol.
                    padPos -= 1; // was out by one...
                    while (padPos > cutPos) { padPos -= 1; sResult += " "; }
                }
                if (asList) { arResult += [ sResult ]; sResult = ""; }
                else sResult += "\n";
                if (colsList) {
                    // Use the next column width in the list.
                    lineIdx += 1; padPos = colsList[lineIdx];
                    if (padPos) {
                        wrapCol = padPos;
                        if (wrapCol < 0) { wrapCol = -wrapCol; bPad = 0; }
                    }
                }

                // Go forward where there should be a space, then
                // go forward until we reach a non-space.
                cutPos += 1;
                while (cutPos < nextPre && sText[cutPos] == " ") cutPos += 1;

                // If we got back to nextPre and nextPre was also nextCR,
                // we need to advance to the next source line.
                if (cutPos == nextCR) break;

                // Start a new output line within this source line.
                numVis = 0;
                srcPos = cutPos;
                continue;

            } else if (nextPre == nextCR) {
                // Did not word-wrap, but we have reached the next CR in
                // the source text, so we must break here.

                // Keep the span before nextCR.
                if (nextCR > srcPos) sResult += sText[srcPos..nextCR-1];

                // Emit a line.
                if (bPad) {
                    // Pad with spaces up to wrapCol.
                    while (overrun < 0) { overrun += 1; sResult += " "; }
                }
                if (asList) { arResult += [ sResult ]; sResult = ""; }
                else sResult += "\n";
                if (colsList) {
                    // Use the next column width in the list.
                    lineIdx += 1; padPos = colsList[lineIdx];
                    if (padPos) {
                        wrapCol = padPos;
                        if (wrapCol < 0) { wrapCol = -wrapCol; bPad = 0; }
                    }
                }

                // Start the next source line.
                break;

            } else {
                // Did not word-wrap or hit end of source line.
                // Consume the whole span and the following colour code.

                sResult += sText[srcPos..nextPre+1];
                numVis += spanLen;

                srcPos = nextPre + 2; // skip prefix and colour code.
            }
        }

        // Skip to the first character of the next source line.
        srcPos = nextCR + 1;
    }

    if (asList) return arResult;
    return sResult[0..-2];
}


[$.globals]
RollDice(pat) {
    d = search(pat, "d");
    if (d < 0) d = search(pat, "D");
    if (d > 0) {
        // MdN or MdN+B
        p = search(pat, "+");
        // deal with + base damage first.
        if (p > 0) {
            base = to_int(pat[p+1..]);
            if (!base || base < 0) base = 0;
        } else {
            base = 0;
            p = length(pat);
        }
        faces = to_int(pat[d+1..p-1]);
        if (!faces || faces < 1) faces = 1;
        rolls = to_int(pat[0..d-1]);
        if (!rolls || rolls < 1) rolls = 1;
        if (rolls > 10) rolls = 10;
        for (i=0; i<rolls; i+=1) {
            base += 1 + random(faces);
        }
        return base;
    } else {
        m = search(pat, "-");
        if (m > 0) {
            // range like 20-30
            min = to_int(pat[0..m-1]);
            if (!min || min < 0) min = 0;
            max = to_int(pat[m+1..]);
            if (!max || max < min) max = min;
            range = max - min;
            if (range == 0) return min;
            // split the range in half and roll 2 dice.
            half = to_int(range * 0.5);
            return min + random(1 + half) + random(1 + range - half);
        } else {
            // just a number.
            val = to_int(pat);
            if (!val || val < 0) val = 0;
            return val;
        }
    }
}

[$.globals]
ParseMatch(phrase, me, all, extras) {
    words = split(lower_case(trim(phrase)), " ");
    // handle 'my' at the start.
    first = 0;
    my = (words[0] == "my");
    if (my) first += 1;
    // handle 'here' at the end.
    last = length(words) - 1;
    here = (words[last] == "here");
    if (here) last -= 1;
    // handle (number) at the end.
    num = to_int(words[last]);
    if (numberp(num)) last -= 1;
    // identify the noun.
    noun = words[last];
    pmatch = (length(noun) >= 3);
    last -= 1;
    // words between [first,last] are adjectives.
    matches = [];
    partial = [];
    if (!here) {
        // find candidates in inventory first.
        foreach (item in me.contents) {
            this.FixNouns(item);
            foreach(word in item.nouns) {
                if (word == noun) matches += [item]; // eek
                else if (pmatch && search(word, noun) == 0) partial += [item]; // eek
            }
        }
    }
    room = me.location;
    if (!my && room) {
        // find candidates in the room second.
        if (extras) {
            // e.g. hidden people in the room you have spotted.
            foreach (item in extras) {
                // verify they are still in the same room!
                // verify hidden, otherwise we will match twice.
                if (item.location == room && item.hidden) {
                    foreach(word in item.nouns) {
                        if (word == noun) matches += [item]; // eek
                        else if (pmatch && search(word, noun) == 0) partial += [item]; // eek
                    }
                }
            }
        }
        // check normal room contents.
        foreach (item in room.contents) {
            this.FixNouns(item);
            if (!item.hidden) {
                foreach(word in item.nouns) {
                    if (word == noun) matches += [item]; // eek
                    else if (pmatch && search(word, noun) == 0) partial += [item]; // eek
                }
            }
        }
    }
    // use the partial matches if there are no exact matches.
    if (!matches) matches = partial;
    // filter the matches with all of the adjectives given.
    keep = [];
    foreach (item in matches) {
        good = 1;
        for (i=first; i<=last; i+=1) {
            adj = words[i];
            // this adj must match one of the adjectives of item.
            ok = 0;
            foreach (word in item.adjectives) {
                if (word == adj || search(word, adj) == 0) {
                    ok = 1;
                    break;
                }
            }
            if (!ok) {
                // this adj did not match; discard the item.
                good = 0;
                break;
            }
        }
        // keep it if every adj matched the item.
        if (good) keep += [item]; // eek
    }
    // select the Nth match if requested.
    if (numberp(num)) {
        if (num < 1) return null;
        num -= 1;
        if (all) return keep[num..num];
        return keep[num];
    }
    if (all) return keep;
    return keep[0];
}

[$.globals]
FixNouns(item) {
    if (clonep(item)) item = parent(item);
    if (!exists(item, "nouns")) {
        id = item.id;
        while (id[-1] == ",") id = id[0..-2];
        if (id) item.nouns = split(id, ",");
    }
    if (!exists(item, "plurals")) {
        id = item.idplural;
        while (id[-1] == ",") id = id[0..-2];
        if (id) item.plurals = split(id, ",");
    }
    if (!exists(item, "adjectives")) {
        id = item.idadj;
        while (id[-1] == ",") id = id[0..-2];
        if (id) item.adjectives = split(id, ",");
    }
}

[$.globals]
pronouns = {
    "male":   { "ns": "he",  "no": "him", "np": "his",  "na": "his",
                "ts": "he",  "to": "him", "tp": "his",  "ta": "his" },
    "female": { "ns": "she", "no": "her", "np": "hers", "na": "her",
                "ts": "he",  "to": "him", "tp": "his",  "ta": "his" }
}

$n: name, $ns: he/she, $no: him/her, $np: his/hers, $na: his/her


[$.globals]
ExpandMessage(msg, vars) {
    ofs = 0;
    res = "";
    me = vars["me"];
    target = vars["target"];
    repeat {
        mark = search(msg, "$", ofs);
        if (mark < 0) {
            // no more placeholders.
            if (ofs) msg = res + msg[ofs..];
            return capitalise(msg);
        }
        // keep the part before the placeholder.
        if (mark > ofs) res += msg[ofs..mark-1];
        // find the end of the placeholder.
        ofs = mark + 1;
        next = msg[ofs];
        if (next == "$") {
            res += next;
            ofs = mark + 2;
            continue;
        }
        end = skip(msg, ofs, "abcdefghijklmnopqrstuvwxyz0123456789");
        if (end < 0) end = length(msg);
        name = msg[ofs..end-1];
        if (msg[end] == ";") end += 1; // allow letters to follow.
        ofs = end;
        switch (name) {
            case "n": if (me) res += me.Name(); break;
            case "ns": if (me) res += this.pronouns[me.gender][name]; break;
            case "no": if (me) res += this.pronouns[me.gender][name]; break;
            case "np": if (me) res += this.pronouns[me.gender][name]; break;
            case "na": if (me) res += this.pronouns[me.gender][name]; break;
            case "t": if (target) res += target.Name(); break;
            case "ts": if (target) res += this.pronouns[target.gender][name]; break;
            case "to": if (target) res += this.pronouns[target.gender][name]; break;
            case "tp": if (target) res += this.pronouns[target.gender][name]; break;
            case "ta": if (target) res += this.pronouns[target.gender][name]; break;
            default: res += replace(vars[name],"#","##"); break;
        }
    }
}


[$.globals]
_items(t, vars) {
  if (arrayp(t)) return t;
  if (mappingp(t)) return keys(t);
  if (stringp(t)) {
    t = this.find_path(t, vars);
  }
  if (objectp(t)) {
    // room, player, mob, container.
    if (arrayp(t.contents)) return t.contents;
    // special case for traversing from rooms to area/domain
    // metadata, which implies contents of the directory above.
    if (object_name(t) == "@meta") t = parent(t);
    // a directory containing rooms, mobs, items.
    items = [];
    foreach (name in t) {
      if (childp(t[name]) && name != "@meta") {
        append(items, t[name]);
      }
    }
    return items;
  }
  return [];
}

[$.globals]
_col_wrap(buf, span, width, wrap) {
  // doing this here means we have to interpret colour codes!
  // it would be better then to emit #[...#] spans for word-wrap.
  first = rsearch(buf, "\n") + 1;
  line = buf[first..];
  lineLen = this.LengthCC(line);
  span = this.PadCC(span, width);
  if (lineLen + width > wrap) return buf + "\n" + span;
  return buf + span;
}


[$.globals]
RenderTemplate(template, vars, user) {
  if (functionp(template)) {
    user.tempRenderTemplate = template;
    return user.tempRenderTemplate(vars, user);
  }
  return "";
}

[$.globals]
CompileTemplate(pat, warn) {
  pos = 0;
  src = "T(v,u){s=\"\";\n";
  scope = "";
  repeat {
    // find the next placeholder.
    start = search(pat, "{", pos);
    if (start < 0) {
      // no more placeholders.
      src += "s+="+encode(pat[pos..])+";\n";
      break;
    }
    if (start > pos) {
      // keep the text before the placeholder.
      src += "s+="+encode(pat[pos..start-1])+";\n";
    }
    if (pat[start+1] == "{") {
      // escape {{ sequence.
      src += "s+=\"{\";\n";
      pos = start + 2;
      continue;
    }
    // find the end of the placeholder.
    pos = search(pat, "}", start);
    if (pos <= start) {
      // missing close brace.
      if (warn) warn("Missing close brace.");
      break;
    }
    // evaluate the placeholder.
    expr = pat[start+1..pos-1];
    pos += 1;
    words = split(expr, " ");
    if (length(words) > 1) {
      // control statements.
      if (warn) warn("("+expr+")\n");
      if (words[0] == "if") {
        if (words[1] == "not") {
          src += "if(!"+this.CompileExpr(words[2])+"){\n";
        } else {
          src += "if("+this.CompileExpr(words[1])+"){\n";
        }
        scope += "i";
      } else if (words[0] == "each") {
        // each item in container
        name = encode(words[1]);
        if (words[2] != "in") {
          if (warn) warn("Missing 'in' for 'each'.");
          src += "if(0){"; scope += "i";
          continue;
        }
        src += "foreach(i in this._items("+this.CompileExpr(words[3])+",v)){v["+name+"]=i;\n";
        scope += "i";
      } else if (words[0] == "end") {
        // end if, end each, etc
        if (scope) {
          if (scope[-1] == "P") {
            // end a padded text span.
            depth = length(scope);
            src += "s=z"+depth+"+this.PadString(s,w"+depth+");\n";
          } else if (scope[-1] == "L") {
            // end a column formatted span.
            depth = length(scope);
            src += "s=this._col_wrap(z"+depth+",s,w"+depth+",u.cols);\n";
          } else {
            src += "}\n";
          }
          scope = scope[0..-2];
        } else {
          if (warn) warn("End without begin.");
        }
      } else if (words[0] == "pad") {
        // begin a padded text span.
        scope += "P";
        depth = length(scope);
        cols = to_int(words[1]); if (!cols) cols = 0;
        // save width in 'w' var, previous output in 'z' var.
        // capture output until end of scope.
        src += "w"+depth+"="+cols+";z"+depth+"=s;s=\"\";\n";
      } else if (words[0] == "cols") {
        // begin a column formatted span.
        scope += "L"; // push type of scope.
        depth = length(scope);
        cols = to_int(words[1]); if (!cols) cols = 0;
        // save width in 'w' var, previous output in 'z' var.
        // capture output until end of scope.
        src += "w"+depth+"="+cols+";z"+depth+"=s;s=\"\";\n";
      } else {
        // x of y etc.
      }
    } else {
      // substitute in a variable.
      src += "s+="+this.CompileExpr(words[0])+";\n";
    }
  }
  if (length(scope)) {
    if (warn) warn("Mismatched scope: "+scope+"\n");
    for (i=0; i<length(scope); i+=1) src += "}\n";
  }
  src += "return s;}\n\n";
  if (warn) warn(src);
  // compile the function.
  res = parse(src,1);
  if (warn && stringp(res)) warn(res);
  return res;
}

[$.globals]
CompileExpr(word) {
  // compile a dot-path access.
  if (word[0] == "@") {
    return "u.find_path("+encode(word)+",v)";
  }
  s = "v";
  pos = 0;
  depth = 0;
  repeat {
    // find the next operator.
    end = skipto(word,pos,".[]");
    if (end < 0) end = length(word);
    if (end > pos) {
      name = word[pos..end-1];
      if (to_string(to_int(name)) == name) {
        s += "[" + name + "]"; // number index
      } else if (name == "id") {
        return "object_name(" + s + ")"; // .id suffix
      } else if (name == "@id") {
        return "object_path(" + s + ")[7..]"; // .id suffix
      } else {
        s += "[" + encode(name) + "]";
      }
    }
    // number indexing can be followed by another number index.
    repeat {
      char = word[end];
      pos = end+1;
      if (char == "[") {
        // number indexing emits v[v[1]] unless we special-case it.
        numPos = pos;
        if (word[pos] == "-") numPos += 1;
        numEnd = skip(word,numPos,"0123456789");
        if (numEnd > numPos && word[numEnd] == "]") {
          s += "[" + word[pos..numEnd-1] + "]";
          end = numEnd+1;
        } else {
          depth += 1;
          s += "[v";
          break;
        }
      } else if (char == "]" && depth > 0) {
        depth -= 1;
        s += "]";
        break;
      } else {
        break; // a dot.
      }
    }
    if (pos >= length(word)) return s;
  }
}

[$.globals]
find_event(obj, name) {
  repeat {
    handler = obj.events[name];
    if (handler) return handler;
    obj = inherit_list(obj)[0];
    if (!obj) return null;
  }
}

// An activity is a list of commands.
// Pre-splitting: store list of [[verb,rest]...]
// Pre-linking: resolve verb and run its pre-parser [[func,arg,arg]...]  (global verbs seqno)
// Pre-compile: resolve verb and run its pre-compiler to produce code.

[$.globals]
DoActivity(me, cmds)
{
  verbs = $.verbs;
  vars = {};
  vars["me"] = me;
  foreach (cmd in cmds) {
    cmd = trim(cmd);
    space = search(cmd, " ");
    if (space > 0) {
      verb = cmd[0..space-1];
      args = cmd[space+1..];
    } else {
      verb = cmd;
      args = "";
    }
    act = verbs[verb];
    if (!act) {
      // fall back to old commands.
      func = me.commands[verb].Command;
      if (func) {
        res = func(args, me);
      } else {
        res = "What?";
      }
    } else {
      // follow direct aliases, used for exits and emote commands.
      alias = act.alias;
      if (alias) {
        vars["@verb"] = act; // capture the starting verb.
        act = verbs[alias];
      }
      func = act.command;
      if (func) {
        // execute a native command.
        res = func(act, args, me vars);
      } else {
        // search command patterns for a match.
        foreach (pat in act.patterns) {
          // pat = [ keywords, cmds ]
          kw = pat[0];
          len = length(params);
          pos = cap = 0;
          name = null;
          for (i=0; i<len;) {
            if (kw[i] == "$") {
              name = kw[i+1]; // name of the var to capture.
              next = kw[i+2]; // look-ahead keyword.
              if (next == "$") {
                // the next keyword is also a var.
                // capture a single word for this var.
              } else {
                // the next keyword is a fixed word.
              }
            }

            if (kw == "$") {
              // the next word is a var name to capture.
              // start capturing words here.
              cap = pos;
              name =
            }
          }
        }
      }
    }
    if (stringp(res)) return res;
  }
}

// The patterns we need to match are:
// verb [sub] [sub..] $text
// verb $var (keyword) $var

CompilePatterns(patterns) {
  s = "";
  foreach (pat in patterns) {
    words = split(pat," ",0,1);
    lvar = null;
    // skip leading spaces, stop if cmd is empty.
    s += "pos=skip(cmd,0,\" \");if(pos<0)return null;\n";
    foreach(word in words) {
      if (word[0] == "$") {
        // match a var: stash the var name for later.
        // if a var is active, emit a capture for one word.
        if (lvar) {
          s += "e=skipto(cmd,pos,\" \");\n"              // find next space
               "if(e<0)e=length(cmd);\n"                 // or end of cmd
               "v["+encode(lvar)+"]=slice(cmd,pos,e);\n" // capture one word
               "pos=e;\n";                               // advance to next word
        }
        lvar = word[1..];
      } else {
        // match a key-word: search forward for this word.
        srch = " "+
        word+" ";
        s += "kw=search(cmd,"+encode(srch)+",pos);\n"
             "if(kw>=0){\n"; // does not match this pattern.
        if (lvar) {
          s += "v["+encode(lvar)+"]=trim(slice(cmd,pos,kw));\n";
          lvar = null;
        }
      }
    }
  }
}

[$.globals]
Pager(me) {
  // Wait for the user to press enter.
  me.Message("#K--more--#n");
  line = me.ReadRawLine();
  if (line) {
    // break out of --more--
    if (line != "q" && line != "quit") {
      res = me.PlayerCommand(line);
      if (res) return res;
    }
    return 1; // stop.
  }
  return 0; // continue
}

[$.globals]
find_path(path, vars) {
    if (length(path) < 2 || path[0] != "@") return null;
    path = replace(path[1..],"^","");
    if (search(path, "$") < 0) {
        // simple path.
        return find_object("$."+path);
    }
    // path with vars.
    bits = split(path, ".");
    len = length(bits);
    for (i=0; i<len; i+=1) {
        seg = bits[i];
        if (seg[0] == "$") {
            val = to_string(vars[seg[1..]] || "");
            bits[i] = replace(val, ".", "");
        }
    }
    return find_object("$."+join(bits, "."));
}

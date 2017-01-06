[$.lib.user.user]
-> $.lib.basic.living

[$.lib.user.user]
PlayerCommand(cmd) {
    // expand aliases.
    limit = 0;
    do {
        pos = skip(cmd); // skip spaces
        space = search(cmd, " ", pos);
        if (space > pos) {
            verb = slice(cmd, pos, space);
        } else {
            if (pos) verb = cmd[pos..]; else verb = cmd;
        }
        alias = this.aliases[verb];
        if (!alias) break;
        if (search(alias, "$$") >= 0) {
            if (space > pos) {
                cmd = replace(alias, "$$", cmd[space+1..]);
            } else {
                cmd = replace(alias, "$$", "");
            }
        } else {
            if (space > pos) cmd = alias + cmd[space..];
            else cmd = alias;
        }
        limit += 1;
    } while (limit < 10);

    // do the resolved command.
    vars = new mapping;
    vars["player"] = this;
    vars["self"] = this; // for $n
    vars["room"] = this.location;
    res = this.DoCommand(cmd, vars, 1);
    if (stringp(res)) this.Message(res);
}



admin = 0
class = $.mud.class.player.warrior
cols = 79
commands = $.lib.cmd.user
creator = 0
cwd = $
editBuffers = null
email = ""
family = null
froggy = 0
lastInput = 0
levelPoints = 10
location = null
loginSocket = null
loginThread = null
lord = 0
name = "-"
noclone = 1
option = {"comments" : 0}
pageHistory = []
pageList = []
password = null
plan = "no plan."
race = $.mud.race.player.human
realName = "Not Specified"
rows = 24
startLocation = $.area.calydynia.city.swSqr
tellHistory = []
tellReply = null
term = "ansi"
title = "the formless blue mass."
username = "-"
portInMsg = " teleports in."
portOutMsg = " teleports out."

DisplayNewPages()
{
    arPageList = this.pageList;
    this.pageList = [];

    this.Message( "" );
    foreach( arPage in arPageList )
    {
        if( arPage[0] == this )
        {
            this.Message( ctime(arPage[2])[0..18] +
                " #MYou page yourself:#n " + arPage[1] );
        }
        else
        {
            this.Message( ctime(arPage[2])[0..18] + " #M" +
                arPage[0].name + " pages you:#n " + arPage[1] );
        }

        if( length(this.pageHistory) < 20 ) this.pageHistory += [ arPage ];
        else this.pageHistory = this.pageHistory[-19..] + [ arPage ];
    }
}

DisplayPageHistory()
{
    arHistory = this.pageHistory;
    nLen = sizeof(arHistory);
    if( nLen )
    {
        sResult = "";
        foreach( arPage in arHistory )
        {
            sResult += ctime(arPage[2])[0..18];
            sMessage = replace( arPage[1], "$", "$$" ); // temp fix for old history
            if( arPage[0] == this ) sResult += " #MYou page yourself:#n " + sMessage + "\n";
            else
            {
                if( arPage[0] ) sName = arPage[0].name; else sName = "unknown";
                sResult += " #M" + sName + " pages you:#n " + sMessage + "\n";
            }
        }
        sResult = sResult[0..-2];
    }
    else sResult = "You have no page history.";

    this.Message( sResult );
}

DisplayTellHistory()
{
    arHistory = this.tellHistory;
    nLen = sizeof(arHistory);
    if( nLen )
    {
        sResult = "";
        foreach( arTell in arHistory )
        {
            oAgent = arTell[0];
            sArgs = arTell[1];
            if( sizeof(arTell) > 2 ) nTime = arTell[2]; else nTime = 0;
            sResult += ctime(nTime)[0..18];
            bSent = (sizeof(arTell) > 3 && arTell[3]);
            if( oAgent == this ) sResult += " #YYou tell yourself:#n " + sArgs + "\n";
            else
            {
                if( bSent ) sResult += " #YYou tell " + oAgent.name + ":#n " + sArgs + "\n";
                else sResult += " #Y" + oAgent.name + " tells you:#n " + sArgs + "\n";
            }
        }
        sResult = sResult[0..-2];
    }
    else sResult = "You have no tell history.";

    this.Message( sResult );
}

[$.lib.user.user]
DropConnection()
{
    // Don't send any user messages directly or indirectly from here
    // Remove the user from the on-line list
    this.loginDaemon.LogoutUser(this);

    // Silently and forcedly remove the user from their location
    if (this.location)
    {
        this.startLocation = this.location;
        this.location.RemoveObject(this);
        this.location = null;
    }

    // Clean up the connection socket
    if (this.loginSocket) close(this.loginSocket);
    this.loginSocket = null;

    // Kill the user thread (may be this thread)
    thUser = this.loginThread;
    this.loginThread = null;
    if (thUser) kill_thread(thUser);
}

IsConnected()
{
    return (this.loginSocket && this.loginThread);
}

Login( ptSocket, passwd )
{
    // Attempt to log in as this user

    // Authenticate the password
    if( !this.password || crypt( passwd, this.password ) == this.password )
    {
        // Check if the user is already logged on
        if( this.loginSocket )
        {
            // Drop the existing connection first
            write_text( ptSocket, "\nYour character is already logged on.\nLet's boot the "
              "other thingy off, shall we?\n" );

            // This forces activity on the TCP socket so it will close
            // if it is idle.
            try {
                write_text( this.loginSocket, "\n\n" );
            }

            this.DropConnection();

            this.loginSocket = ptSocket;
            this.loginDaemon.Inform( this.name + " reconnects" );
        }
        else
        {
            this.loginSocket = ptSocket;
            this.loginDaemon.Inform( this.name + " enters " + this.mudName );
        }

        // Start the main user threa
        this.loginThread = thread this.UserMain();

        return 1;
    }

    return 0;
}

[$.lib.user.user]
Look()
{
    // Look around the environment
    here = this.location;
    if( !here ) return "You are in limbo.";

    result = "\n";

    // Include the room id for imms
    if (this.creator) {
        result = "#K"+this.RoomId(here)+" ("+here.xpos+","+here.ypos+","+here.zpos+")#n\n";
    }

    // Determine the valid exit directions from this room
    NW = NE = SW = SE = "--";
    N = S = "-";
    E = W = U = D = " ";
    arExitList = here.exits;
    foreach (arExit in arExitList) {
        if (arExit[2].hidden) continue; // exit class hidden.
        switch (arExit[0]) {
            case "northwest":  NW = "#oNW#n"; break;
            case "northeast":  NE = "#oNE#n"; break;
            case "southwest":  SW = "#oSW#n"; break;
            case "southeast":  SE = "#oSE#n"; break;
            case "north":       N = "#oN#n"; break;
            case "south":       S = "#oS#n"; break;
            case "east":        E = "E"; break;
            case "west":        W = "W"; break;
            case "up":          U = "U"; break;
            case "down":        D = "D"; break;
        }
    }

    // Build the compass based on valid exit directions
    nCompassWidth = 7;
    sCompassA = NW  + "-" + N + "-" + NE;
    sCompassB = " #o" + W + U + "#K+#o" + D + E + " #n";
    sCompassC = SW + "-" + S + "-" + SE;

    // Build the icon for this room (fixed size 14 characters)
    nIconWidth = 14;

    oArea = here.area;
    worldSys = this.worldSys;

    // Build the map for this room (maps are 9x5 characters + border)
    nMapWidth = 11;
    arMiniMap = here.GenerateMap(this);
    /*
    arMiniMap = here.minimap;
    if( !arMiniMap ) {
        if( oArea ) {
            arMiniMap = split( oArea.RenderMap( here, 9, 5 ), "\n" );
            here.minimap = arMiniMap;
        }
        else arMiniMap = here.emptymap;
    }
    */

    // Calculate the header width
    nCols = this.cols;
    nHeader = nCols - nIconWidth - nCompassWidth - nMapWidth - 2;

    sTitle = here.Name();
    nSpace = nHeader - length(sTitle) - 2;
    nPad = to_int(nSpace / 2);

    result += worldSys.sky1 + " " + this.RepeatString( "-", nPad ) + " #c" +
      sTitle + "#n " + this.RepeatString( "-", nSpace - nPad ) + sCompassA +
      " @=========@\n";

    if( oArea ) sInfo = oArea.name; else sInfo = "Somewhere";
    sInfo += ", " + here.DescribeDaylight() + ", " + here.DescribeWeather();

    nSpace = nHeader - length(sInfo);
    nPad = to_int(nSpace / 2);

    result += worldSys.sky2 + " " + this.RepeatString( " ", nPad ) + sInfo +
      this.RepeatString( " ", nSpace - nPad ) + sCompassB +
      " |" + arMiniMap[0] + "#n|\n";

    result += worldSys.dayLine + " " + this.RepeatString( "-", nHeader ) + sCompassC +
      " |" + arMiniMap[1] + "#n|\n";

    nWrap = nCols - 12; // map is 11 wide plus 1 space

    blankLine = this.RepeatString( " ", nWrap );
    result += blankLine + " |" + arMiniMap[2] + "#n|\n";

    sDesc = here.Describe( this );

    // Word-wrap using specific line lengths (the last length repeats)
    // Use negative length to turn off line padding with spaces!
    // lineCols = [ nWrap, nWrap, nWrap, -nWrap, nCols ];
    lineCols = [ nWrap, nWrap, nWrap, -nWrap ];
    arDesc = this.Wrap( sDesc, lineCols, 1, 1 );

    descIdx = 0;
    mapIdx = 3;
    while (mapIdx <= 5) {
        // Next line of the description.
        line = arDesc[descIdx]; if (!line) line = blankLine;
        result += line;
        descIdx += 1;
        // Next line of the map.
        if (mapIdx < 5) result += " #n|" + arMiniMap[mapIdx] + "#n|\n";
        else result += " #n@=========@\n";
        mapIdx += 1;
    }

    // Rest of the lines of the description.
    while (descIdx < length(arDesc)) {
        result += arDesc[descIdx] + "\n";
        descIdx += 1;
    }

    if (this.creator) {
        if (length(here.comments)) result += "\n#K" + here.comments + "\n";
    }

    this.MessageNoWrap( result );
}

[$.lib.user.user]
Message( sMessage, oAgent, mContext )
{
    // Word-wrap the message to the configured screen width
    sMessage = this.terminalDaemon.Wrap( sMessage, this.cols );

    // Resolve colour codes in the message
    if( search( sMessage, "#" ) >= 0 ) {
        sMessage = this.terminalDaemon.ResolveColour( sMessage + "#n", this.term );
    }

    // Write the message to the user
    this.RawMessage( sMessage + "\n" );
}

[$.lib.user.user]
MessageWrap( sMessage )
{
    // Word-wrap the message to the configured screen width
    sMessage = this.terminalDaemon.Wrap( sMessage, this.cols );

    // Resolve colour codes in the message
    if( search( sMessage, "#" ) >= 0 ) {
        sMessage = this.terminalDaemon.ResolveColour( sMessage + "#n", this.term );
    }

    // Write the message to the user
    this.RawMessage( sMessage + "\n" );
}

MessageNoWrap( sMessage )
{
    // Check if the message contains any colour
    if( search( sMessage, "#" ) >= 0 )
    {
        // Resolve all colour codes in the message
        sMessage = this.terminalDaemon.ResolveColour( sMessage + "#n", this.term );
    }

    // Write the message to the user
    this.RawMessage( sMessage );
}

Name()
{
    return this.name;
}

[$.lib.user.user]
NetDead() {
    // Wrap these in try-catches in case the socket drops out
    try {
        this.loginDaemon.Inform( this.Name() + " loses grip on reality" );
    }
    this.DropConnection();
}

[$.lib.user.user]
Quit() {
    // Wrap these in try-catches in case the socket drops out
    try {
        // Display a quit message
        this.Message( "\nAlright I'll see you later then.\n\n" );
    }
    try {
        this.loginDaemon.Inform( this.Name() + " leaves " + this.mudName );
    }
    return this.DropConnection();
}

RawMessage( sMessage )
{
    // Write the message out to the user
    try write_text( this.loginSocket, sMessage );
}

ReceivePage( oUser, sMessage )
{
    arPage = [ oUser, sMessage, time(), 0 ];

    if( this.loginSocket && oUser != this )
    {
        // Add to page history
        if( length(this.pageHistory) < 20 ) this.pageHistory += [ arPage ];
        else this.pageHistory = this.pageHistory[-19..] + [ arPage ];

        // Tell the user interactively
        this.Message( "#M" + oUser.name + " pages you:#n " + sMessage );

        // Tell everyone else
        if( this.location ) this.location.Broadcast( "#M" + this.name +
            " beeps three times and explodes.", this );

        return this.name + " has received your message.";
    }
    else
    {
        // Add this to the page list
        this.pageList += [ arPage ];

        if( oUser == this )
        {
            if( this.location ) this.location.Broadcast( "#M" + this.name +
                " beeps a few times and looks amused.", this );

            return "You amuse those around you by sending yourself silly messages.";
        }

        return "You leave a message for " + this.name + ".";
    }
}

ReceiveTell( oAgent, sMessage )
{
    // Save the sending user
    this.tellReply = oAgent;

    // Add this to the tell history
    arAppend = [ [ oAgent, sMessage, time(), 0 ] ];
    if( length(this.tellHistory) < 20 ) this.tellHistory += arAppend;
    else this.tellHistory = this.tellHistory[-19..] + arAppend;

    // Tell the user
    if( oAgent == this ) sResult = "#YYou tell yourself:#n " + sMessage;
    else sResult = "#Y" + oAgent.name + " tells you:#n " + sMessage;

    this.Message( sResult );
}

ReportError( err, bUncaught )
{
    if( this.loginSocket )
    {
        sResult = this.DescribeError( err, bUncaught );

        try
        {
            // Display the error to this user
            this.MessageNoWrap( "#K" + replace( sResult, "#", "##" ) );
        }
        catch( ierr )
        {
            if( this.loginSocket ) write_text( this.loginSocket, sResult );
        }
    }
}

REMOVED :::: ResolveMessage( sMessage, oAgent, mContext )
{
    if( !oAgent ) oAgent = this;
    if( !mContext ) mContext = [];
    else if( !arrayp(mContext) ) mContext = [mContext];

    nSize = sizeof(mContext);
    nLen = length(sMessage);

    sResult = "";
    sDelim = "$";
    sInDelim = ":";
    nStart = 0;

    repeat
    {
        // Find the next starting delimiter
        nOfs = search( sMessage, sDelim, nStart );
        if( nOfs < 0 ) return sResult + sMessage[nStart..-1]; // stop; finished

        // Keep the segment before the delimiter
        if( nOfs ) sResult += sMessage[nStart..nOfs - 1];

        // Find the matching end delimiter
        nEnd = search( sMessage, sDelim, nOfs + 1 );
        if( nEnd < 0 ) return sResult; // stop; unmatched delimiter
        nStart = nEnd + 1;

        nOfs += 1;
        nEnd -= 1;

        // Check for the capitalise prefix
        if( sMessage[nOfs..nOfs] == "^" )
        {
            nOfs += 1;
            bCapitalise = 1;
        }
        else bCapitalise = 0;

        // Search for an internal delimiter
        nSplit = search( sMessage, sInDelim, nOfs );
        if( nSplit > nOfs && nSplit < nEnd )
        {
            sTarget = sMessage[nOfs..nSplit - 1];
            sAttrib = sMessage[nSplit + 1..nEnd];
        }
        else
        {
            sTarget = sMessage[nOfs..nEnd];
            sAttrib = "";
        }

        sAppend = null;
        nIndex = 0;
        switch( sTarget )
        {
            case "agent":
                nIndex = 0;
                break;

            case "target":
                nIndex = 1;
                break;

            case "mudname":
                sAppend = this.mudName;
                break;

            case "":
                sAppend = "$";
                break;

            default:
                nIndex = to_int(sTarget);
                break;
        }

        if( !sAppend )
        {
            if( nIndex == 0 )
            {
                sAppend = oAgent.Name();
            }
            else if( nIndex > 0 && nIndex <= nSize )
            {
                switch( sAttrib )
                {
                    case "":
                    case "name":
                        sAppend = this.ListItems( mContext[nIndex - 1], oAgent );
                        break;

                    default:
                        continue;
                }
            }
            else
            {
                // Also trim trailing spaces?
                continue;
            }
        }

        if( bCapitalise ) sResult += capitalise(sAppend);
        else sResult += sAppend;
    }
}

SendTell( oUser, sMessage )
{
    // Send the tell to the user
    oUser.ReceiveTell( this, sMessage );

    // Add this send to the tell history
    if( oUser != this )
    {
        arAppend = [ [ oUser, sMessage, time(), 1 ] ];
        if( length(this.tellHistory) < 20 ) this.tellHistory += arAppend;
        else this.tellHistory = this.tellHistory[-19..] + arAppend;

        this.Message( "#YYou tell " + oUser.name + ":#n " + sMessage );
    }
}

[$.lib.user.user]
ReadRawLine() {
    line = read_line(this.loginSocket);
    this.lastInput = time();
    return line;
}

[$.lib.user.user]
ReadLine(prompt) {
    repeat {
        if (prompt) this.MessageNoWrap(prompt);
        line = trim(read_line(this.loginSocket));
        this.lastInput = time();
        if (line[0] != "!") return line;
        this.DoFallback(line[1..]);
    }
}

[$.lib.user.user]
RequestLine(prompt) {
    repeat {
        line = this.ReadLine(prompt);
        if (line != "" ) return line;
    }
}

[$.lib.user.user]
DoFallback(cmd) {
    // fallback command processing, in case the normal
    // command processing is broken by a change.
    cmd = trim(cmd);
    pos = search(cmd," ");
    if (pos > 0) {
        verb = cmd[0..pos-1];
        args = cmd[pos+1..];
    } else {
        verb = cmd;
        args = "";
    }
    if (functionp(this.commands[verb].Command)) {
        res = this.commands[verb].Command(args, this);
    } else if (functionp($.verbs[verb].func)) {
        res = $.verbs[verb].func(args, this);
    } else {
        res = "What?";
    }
    if (stringp(res)) this.Message(res);
}

[$.lib.user.user]
WritePrompt() {
    // Display the user's prompt.
    exits = "limbo";
    here = this.location;
    if (here) exits = here.GetExitSummary();
    prompt = "\n<#g"+this.hp+"#nhp #m"+this.en+"#ne #g"+this.mv+"#nmv "+exits+"> ";
    this.MessageNoWrap(prompt);
}

[$.lib.user.user]
UserMain()
{
    // Main thread for a logged in user.

    try {
        // Display the sMessage of the day
        this.Message(this.loginDaemon.GetNews(this));

        // Move the user into their start room
        if (!this.location) this.Move(this.startLocation, null, null, this);
        else this.Look();

        // Display any outstanding pages
        this.DisplayNewPages();
    }
    catch(err) {
        this.ReportError(err);
    }

    repeat {
        cmd = "";

        try {
            this.WritePrompt();
        } catch(err) {
            this.ReportError(err);
        }

        try {
            // Wait for a user command
            cmd = trim(read_line(this.loginSocket));
            this.lastInput = time();
        } catch(err) {
            // User's connection has been dropped
            return this.NetDead();
        }

        try {
            if (cmd) {
                if (cmd[0] != "!") {
                    this.PlayerCommand(cmd);
                } else {
                    this.DoFallback(cmd[1..]);
                }
            }
        }
        catch(err) {
            this.ReportError(err);
        }
    }
}


-- REMOVED :::: new pattern rendering stuff

[$.lib.user.user]
EvalExpr(pat, start, end, vars) {
  this.RawMessage("("+pat[start..end]+") ");
  path = split(pat, ".");
  val = vars;
  foreach (name in path) {
    if (objectp(val) || mappingp(val)) val = val[name]; else val = null;
  }
  return val;
}

[$.lib.user.user]
RenderPattern(pat, vars, pos) {
  if (!pos) pos = 0;
  len = length(pat);
  line = "";
  result = [0];
  align = 0; // left.
  repeat {
    // find the next placeholder.
    start = search(pat, "{", pos);
    if (start < 0) {
      // no more placeholders.
      if (pos < len) {
        line += pat[pos..];
        this.RawMessage("\""+pat[pos..]+"\" ");
      }
      break;
    }
    if (start > pos) {
      // keep the text before the placeholder.
      line += pat[pos..start-1];
      this.RawMessage("\""+pat[pos..start-1]+"\" ");
    }
    // find the end of the placeholder.
    pos = search(pat, "}", start+1) + 1; if (pos == 0) pos = len;
    // evaluate the placeholder.
    switch (pat[start+1]) {
    case "{":
      // escape open brace
      line += "{";
      pos = start + 2;
      this.RawMessage("{{ ");
      continue;
    case "%":
      // formatting commands.
      switch (pat[start+2]) {
        case "}":
          // end of layout block.
        this.RawMessage("FMT] ");
        case "B":
          // break line.
          this.RawMessage("BR ");
          if (line) { result += [line]; line = ""; }
        case "F":
          // float left/right.
          switch (pat[start+3]) {
            case "L":
              this.RawMessage("[LEFT ");
              break;
            case "R":
              this.RawMessage("[RIGHT ");
              break;
          }
          break;
        case "C":
          // center alignment.
          this.RawMessage("[CENTER ");
          break;
      }
      break;
    case "?":
      if (pos == start+3) {
        this.RawMessage("IF] ");
      } else {
        this.RawMessage("[IF ");
        val = this.EvalExpr(pat, start+2, pos-2, vars);
      }
      break;
    case ":":
      this.RawMessage("ELSE ");
      break;
    case "@":
      if (pos == start+3) {
        this.RawMessage("FOR] ");
      } else {
        colon = search(pat, ":", start+2); if (colon < 0 || colon > pos) colon = pos;
        name = pat[colon+1..pos-2];
        this.RawMessage("[FOR {"+name+"} ");
        val = this.EvalExpr(pat, start+2, colon-1, vars);
        subs = new mapping;
        if (name) { subs[name] = val; }
        if (arrayp(val)) {
        }
      }
      break;
    default:
      val = this.EvalExpr(pat, start+1, pos-2, vars);
      break;
    }
  }
  result[0] = pos;
  return result;
}

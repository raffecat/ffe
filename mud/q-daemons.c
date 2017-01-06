[$.lib.daemon]

[$.lib.daemon.combat]
-> $.lib.basic.daemon
combatGroups = []
mainThread = null
rate = 4

JoinCombat( oAttacker, oTarget )
{
    // If both are in combat groups, what happens?
}

BeginCombat( oCombatant )
{
    // Check if the attacker is already in combat
    bFound = 0;
    foreach( oEntry in this.combatGroups )
    {
        if( oEntry == oCombatant ) { bFound = 1; break; }
    }
    if( bFound == 0 )
    {
        // Add the combatant to the combat list
        this.combatGroups += [ oCombatant ];
    }
}

EndCombat( oCombatant )
{
    // Remove this combatant from the combat list
    remove( this.combatGroups, oCombatant );
}

Main()
{
    repeat
    {
        // Wait for the next round
        sleep( this.rate );

        // Perform all rounds of combat
        remove( this.combatGroups, null );
        foreach( oAttacker in this.combatGroups )
        {
            oAttacker.CombatRound();
        }
    }
}

[$.lib.daemon.httpd]
-> $.lib.basic.daemon
errorRoot = $.web.error
httpPort = 4000
listenPort = null
mainThread = null
webRoot = $.web

CompilePage( sContent, sPath, sName )
{
    sFunc = sName + "( httpd, connection, request, query, querystring )\n{\nresponse = \"\";\n\n";

    // Convert the entire page into a function
    nStart = 0;
    repeat
    {
        // Find the next embedded code tag
        nOpen = search( sContent, "<?ffe", nStart );
        if( nOpen >= 0 )
        {
            // Append the content before the open tag
            sFunc += "response += \"" + sContent[nStart..nOpen-1] + "\";\n";
            nStart = nOpen + 5;

            // Find the closing code tag
            nClose = search( sContent, "?>", nStart );
            if( nClose >= 0 )
            {
                // Append the embedded code fragment
                sFunc += sContent[nStart..nClose-1] + "\n";
                nStart = nClose + 2;
            }
        }
        else
        {
            // Append the rest of the content
            sFunc += "response += \"" + sContent[nStart..] + "\";\n";
            break;
        }
    }

    sFunc += "\nreturn response;\n}\n";

    // Compile the function
    arResult = parse( sFunc );

    // If errors, return the error report
    sErrors = arResult[1];
    if( sErrors ) return sErrors + "\n----\n" +
        replace( replace( sFunc, ">", "&gt" ), "<", "&lt" );

    // Return the compiled page
    return arResult[0];
}

ExpandQuery( sQuery )
{
    // Split the query into fields
    mpQuery = new mapping;
    arFields = split( sQuery, "&" );
    foreach( sField in arFields )
    {
        nEqual = search( sField, "=" );
        if( nEqual > 0 )
        {
            sFieldName = sField[0..nEqual-1];
            sFieldVal = sField[nEqual+1..];
        }
        else
        {
            sFieldName = sField;
            sFieldVal = "";
        }
        if( exists( mpQuery, sFieldName ) )
        {
            mExisting = mpQuery[sFieldName];
            if( arrayp(mExisting) ) mpQuery[sFieldName] += [ sFieldVal ];
            else mpQuery[sFieldName] = [ mExisting, sFieldVal ];
        }
        else mpQuery[sFieldName] = sFieldVal;
    }
    return mpQuery;
}

Main()
{
    // Listen for incoming connections forever
    repeat
    {
        try
        {
            // Accept a connection
            ptConn = accept( this.listenPort );
        }
        catch( err )
        {
            console( "--> Failed to accept connection on httpd port" );

            // Reset the daemon
            this.listenPort = null;
            this.Reset();

            console( "--> Successfully reset the http daemon" );

            continue;
        }

        try
        {
            // Spawn a thread to handle the request
            thread this.Request( ptConn );
        }
    }
}

Request( ptConn )
{
    // Retrieve the request from the client
    sReq = lower_case( read_line( ptConn ) );
    console( "[httpd] " + sReq );

    if( sReq[0..4] == "get /" )
    {
        // Strip off the GET keyword
        sReq = sReq[5..];

        // Strip off the http version suffix
        nPos = search( sReq, " http/" );
        if( nPos > 0 ) sReq = sReq[0..nPos-1];
        else if( nPos == 0 ) sReq = "";

        // Process the query string if present
        nMark = search( sReq, "?" );
        if( nMark >= 0 )
        {
            sQuery = sReq[nMark+1..];
            mpQuery = this.ExpandQuery( sQuery );
            if( nMark ) sReq = sReq[0..nMark-1]; else sReq = "";
        }
        else
        {
            sQuery = "";
            mpQuery = new mapping;
        }

        // Convert the slash-delimited path into dot-delimited
        sReq = replace( sReq, ".", "_" );
        sReq = replace( sReq, "/", "." );

        // Add the default page name to the path if necessary
        if( sReq == "" || sReq[-1..-1] == "." ) sReq += "index";

        // Resolve the path into a user home directory
        if( sReq[0..0] == "~" )
        {
            // Build a path relative to the user's web dir
            nFirst = search( sReq, "." );
            if( nFirst >= 0 )
            {
                sRoot = "$.home." + sReq[1..nFirst] + "web.";
                sReq = sRoot + sReq[nFirst+1..];
            }
            else
            {
                sRoot = "$.home." + sReq[1..] + ".web.";
                sReq = sRoot + "index";
            }
        }
        else
        {
            sRoot = object_path( this.webRoot );
        }

        // Check if the parent directory exists
        console( "[http] " + sReq );
        oPair = this.FindPathName( sReq, this.webRoot );
        if( !oPair )
        {
            this.WritePage( ptConn, this.errorRoot.e404, "404 File Not Found" );
        }
        oContext = oPair[0];
        sName = oPair[1];

        // Check if the path contains illegal parent paths
        if( object_path(oContext)[0..length(sRoot)-1] != sRoot )
        {
            this.WritePage( ptConn, this.errorRoot.e404, "404 File Not Found" );
        }

        // Check if the page exists
        if( !exists( oContext, sName ) )
        {
            this.WritePage( ptConn, this.errorRoot.e404, "404 File Not Found" );
        }

        // Retrieve the page text
        mContent = oContext[sName];
        if( !stringp(mContent) && !functionp(mContent) )
        {
            // Write a server error back to the client
            this.WritePage( ptConn, replace( this.errorRoot.e500, "***ERROR***",
                "<em>The page content was of an invalid type.</em>" ),
                "500 Internal Server Error" );
        }

        // Check if the page is a compiled application
        if( functionp(mContent) )
        {
            try
            {
                // Execute the compiled application
                mContent = mContent( this, ptConn, sReq, mpQuery, sQuery );
            }
            catch( err )
            {
                // Produce an error page
                this.WritePage( ptConn, replace( this.errorRoot.e500, "***ERROR***",
                    this.DescribeError( err, 0 ) ), "500 Internal Server Error" );
            }
        }

        // Check if the page contains embedded ffe code
        if( search( mContent, "<?ffe" ) >= 0 )
        {
            sCompiled = sName + "_";
            mCached = null;
            if( exists( oContext, sCompiled ) )
            {
                // Retrieve the cached page function
                mCached = oContext[sName];
            }

            if( !functionp(mCached) )
            {
                // Compile the page
                mCached = this.CompilePage( mContent, sReq, sName );

                // Store the compiled page for next time
                if( functionp(mCached) )
                {
                    oContext[sCompiled] = mCached;
                    mCached = oContext[sCompiled]; // bound function
                }
                else
                {
                    // Produce an error page
                    this.WritePage( ptConn, replace( this.errorRoot.e500, "***ERROR***",
                        mCached ), "500 Internal Server Error" );
                }
            }

            // Execute the compiled page
            try
            {
                // Execute the precompiled page
                mContent = mCached( this, ptConn, sReq, mpQuery, sQuery );
            }
            catch( err )
            {
                // Produce an error page
                this.WritePage( ptConn, replace( this.errorRoot.e500, "***ERROR***",
                    this.DescribeError( err, 0 ) ), "500 Internal Server Error" );
            }
        }

        // Check if a valid page was generated
        if( stringp(mContent) && length(mContent) )
        {
            // Write the web page back to the client
            this.WritePage( ptConn, mContent, "200 OK" );
        }

        // Write a server error back to the client
        // TODO: there is a standard error code for this
        this.WritePage( ptConn, replace( this.errorRoot.e500, "***ERROR***",
            "<em>No content was generated for this page.</em>" ),
            "500 Internal Server Error" );
    }
    else
    {
        this.WritePage( ptConn, replace( this.errorRoot.e500, "***ERROR***",
            "<em>An unsupported request was received from the client.</em>" ),
            "500 Internal Server Error" );
    }
}

Reset()
{
    // Re-open the server port if it has been closed
    if( !this.listenPort )
    {
        console( "--> httpd server port has been closed" );

        // Open a server port
        this.listenPort = listen( this.httpPort );

        console( "--> httpd listening on " + this.httpPort );
    }

    // Restart the main thread if stopped
    if( !this.mainThread ) this.mainThread = thread this.Main();
}

Startup()
{
    // Kill the old main thread
    if( this.mainThread )
    {
        kill_thread( this.mainThread );
        this.mainThread = null;
    }

    // Open a server port
    this.listenPort = listen( this.httpPort );

    // Start the main thread
    this.mainThread = thread this.Main();

    console( "--> httpd listening on " + this.httpPort );
}

WritePage( ptConn, sContent, sStatus )
{
    // Generate HTTP status
    sStatusLine = "HTTP/1.1 " + sStatus + "\n";

    // Generate HTTP headers
    sTime = ctime();
    sHeader =
        "Date: " + sTime + " GMT\n"
        "Server: Quintiqua IWS 1.0\n"
        "Connection: close\n"
        "Content-Type: text/html\n"
        "\n";

    // Write the page to the client and close the connection
    write_text( ptConn, sStatusLine + sHeader + sContent );

    // Close the connection and drop the thread
    close( ptConn );
    exit;
}

[$.lib.daemon.login]
-> $.lib.basic.daemon
adminNews = ""
creatorNews = ""
generalNews = "#YGeneral News#n\n\nQuintiqua is currently still under development, so we still can't really\naccomodate players. However, once the world is complete, we look forward\nto seeing you. If you wish to lend a hand creating the world, talk to the\nnearest admin.\n"
loginScreen = "      #o                           _\n      #o                          (#B+#o)#R    .,\n      #r                          :;;.,;'#R  '#r;;.#R,.'\n      #o                          |\|  #R ';,.\n      #o                        __#o|\|#o__     #r';',;;,, \n      #o                       (___#RQ#o___)           #R';,.,             \n      #g                  '       #W| #W|  #g'               _   \n      #g      /'\\ \\ \\ \\ \\/\\ #W|#W:#W|#g \\  /'\\ \\ \\  < \, \n      #g     || || || || || || || #W|#W|#W|#g || || || || ||  /-|| \n      #g     || || || || || || || #W|#W|#W|#g || || || || || (( || \n      #g     \\,|| \\/\\ \\ \\ \\ #W|#W|#W|#g \\ \\,|| \\/\\  \/\\ \n      #o     --#g ||#o -------------  #W|#W|#W|#o -----#g ||#o ------------\n      #g        '/`               #W|#W|#W|#g       '/`             \n      #W                          |#W|#W|             \n      #W                          |#W|#W|\n      #W                          |#W|#W|\n      #W                          :#W|#W;\n      #W                           :#n\n\n     http://quintiqua.dark-rune.com, quintiqua.dark-rune.com:7000 \n"
mainThread = null
mainThreadAlt = null
mudPort = 7000
mudPortAlt = null
newUser = $.lib.user.user
newClass = $.mud.class.player.warrior
newRace = $.mud.race.player.human
newuserScreen = "Many millenia ago, the ancients roamed the world under the care of the\ntwin elder gods. After the great cataclysm new races emerged and lived\nin peace and war for millenia more. Now the winds of change are upon\nthe land once more...\n\nQuintiqua, a realm of dreams and nightmares. Where light and dark battle\neternally for supremacy, and mere mortals are caught in conflicts between\nthe great powers.\n\nWelcome to Quintiqua, a Multi User Dimension. While here, you can learn\nthe art of magic, join a guild, go raiding with the Org, search the ancient\ncities of the Acillia or just relax in a tavern with friends. The world\nis open for you.\n\n#OQuintiqua is currently still under development, so we still can't really\naccomodate players. However, once the world is complete, we look forward\nto seeing you. If you wish to lend a hand creating the world, talk to the\nnearest admin.#n\n"
onLineList = []
passwdScreen = "Many millenia ago, the ancients roamed the world under the care of the\ntwin elder gods. After the great cataclysm new races emerged and lived\nin peace and war for millenia more. Now the winds of change are upon\nthe land once more...\n\nQuintiqua, a realm of dreams and nightmares. Where light and dark battle\neternally for supremacy, and mere mortals are caught in conflicts between\nthe great powers.\n"
randomColours = ["#r", "#g", "#b", "#c", "#m", "#o", "#R", "#G", "#B", "#C"]
userList = $.save.user
skipIAC = { "\255": 1, "\251": 3, "\252": 3, "\253": 3, "\254": 3, "\250": 250 }

exec $.lib.daemon.login.skipIAC = new mapping;
exec $.lib.daemon.login.skipIAC["\255"]= 1;
exec $.lib.daemon.login.skipIAC["\251"]= 3;
exec $.lib.daemon.login.skipIAC["\252"]= 3;
exec $.lib.daemon.login.skipIAC["\253"]= 3;
exec $.lib.daemon.login.skipIAC["\254"]= 3;
exec $.lib.daemon.login.skipIAC["\250"]= 250;
cat $.lib.daemon.login.skipIAC

set $.lib.daemon.login.newClass = $.mud.class.player.warrior
set $.lib.daemon.login.newRace = $.mud.race.player.human

ReadLine(socket)
{
    line = read_line(socket);
    repeat {
        pos = search(line, "\255"); // Telnet IAC
        if (pos < 0) break;
        cmd = line[pos+1];
        // Skip 1 (IAC-IAC) or 3 (IAC-WILL|WONT|DO|DONT-OPTION) bytes.
        // In the general case, skip 2 (IAC-ANY)
        skip = this.skipIAC[cmd];
        if (!skip) skip = 2;
        if (!numberp(skip)) write(socket, "((skip NaN))");
        if (skip == 250) {
            // Special case: IAC-SB ... IAC-SE
            endPos = search(line, "\255\240"); // Telnet IAC
            if (endPos < 0) {
                write(socket, "((SE missing))");
                return "";
            }
            sbLen = endPos + 2 - pos;
            write(socket, "((SB-SE:"+sbLen+"))" );
            if (pos > 0) line = line[0..pos-1] + line[endPos+2..];
            else line = line[endPos+2..];
        } else {
            write(socket, "((IAC:"+skip+"))" );
            // keep the parts before and after the telnet command.
            if (pos > 0) line = line[0..pos-1] + line[pos+skip..];
            else line = line[pos+skip..];
        }
    }
    return trim(line);
}

Broadcast( sMessage )
{
    // Send a sMessage to all on-line users
    foreach( oUser in this.Users() )
    {
        oUser.Message( sMessage );
    }
}

CreateUser( ptSocket, sUsername )
{
    // Display the new user screen
    sTitle = "\n" + this.newuserScreen + "\n";
    sTitle = this.terminalDaemon.ResolveColour( sTitle + "#n", "ansi" );
    write_text( ptSocket, sTitle );

    // Ask for (and confirm) the new password
    repeat
    {
        write_text( ptSocket, "Enter a password of at least 5 letters (not hidden!): " );
        sPass = this.ReadLine( ptSocket );
        if( sPass == "" || length( sPass ) >= 5 ) break;

        write_text( ptSocket, "\nYour password must be at least 5 letters in length.\n\n" );
    }
    if( sPass == "" )
    {
        write_text( ptSocket, "\n" );
        return 0;
    }
    write_text( ptSocket, "Please confirm your password: " );
    sPass2 = this.ReadLine( ptSocket );
    if( sPass != sPass2 )
    {
        write_text( ptSocket, "\nYour passwords do not match. Try again.\n\n" );
        return 0;
    }

    // Ask for a gender
    write_text( ptSocket, "\n" );
    sGender = this.RequestOption( ptSocket, "Please choose your character's gender (m/f): ",
      [ "m", "f" ], "\nYou must select a gender for your new character.\n"
      "The valid genders on " + this.mudName + " are (m)ale and (f)emale.\n" );
    if( sGender == "f" ) sGender = "female"; else sGender = "male";

    // Allow the user to select a race and class
    //oRace = ChooseRace( ptSocket, sUsername );
    //if( !oRace ) return 0;

    //oClass = ChooseClass( ptSocket, sUsername );
    //if( !oClass ) return 0;

    oRace = this.newRace;
    oClass = this.newClass;

    // Ask for the user's email address
    write_text( ptSocket, "\nFor security reasons, " + this.mudName + " requires a valid "
      "email address.\nYou can hide your address from other players by preceeding "
      "it with\na '#' character (e.g. #user@somewhere.com).\n" );
    repeat
    {
        sEmail = this.RequestLine( ptSocket, "Enter your email address: " );
        if( search( sEmail, "@" ) > 0 ) break;

        write_text( ptSocket, "\nThat is not a valid email address.\n" );
    }

    // Ask for the user's real name
    write_text( ptSocket, "\nIf you do not mind, please enter your real name.\n"
      "Note that this information will be visible to other players.\n" );
    write_text( ptSocket, "Please enter your real name (optional): " );
    sRealName = this.ReadLine( ptSocket );

    // Create the new user
    write_text( ptSocket, "\nCreating character...\n\n");
    oUser = new object( sUsername, this.userList );
    add_inherit( oUser, this.newUser );
    oUser.username = sUsername;
    oUser.password = crypt( sPass );
    oUser.id = sUsername;
    oUser.name = capitalise( lower_case(sUsername) );
    oUser.gender = sGender;
    oUser.race = oRace;
    oUser.class = oClass;
    oUser.email = sEmail;
    oUser.realName = sRealName;

    // Now log the new user in
    if( oUser.Login( ptSocket, sPass ) )
    {
        // Add the user to the on-line list
        this.LoginUser( oUser );
    }
    else
    {
        write_text( ptSocket, "\nOops something went wrong.\n\n" );
        close( ptSocket );
    }
    exit;
}

[$.lib.daemon.login]
FindUserExact( sName )
{
    if( length( sName ) < 1 ) return null;
    sName = lower_case( sName );
    foreach( oUser in this.Users() )
    {
        sUsername = object_name(oUser);
        if( sUsername == sName ) return oUser; // exact
    }
    return null;
}

[$.lib.daemon.login]
FindImmExact(name) {
    if (!name) return null;
    name = lower_case(name);
    foreach (user in this.Users()) {
        username = object_name(user);
        if (username == name && user.creator) return user; // exact
    }
    return null;
}


[$.lib.daemon.login]
FindImm(name) {
    if (!name) return null;
    name = lower_case(name);
    len = length(name) - 1;
    found = null;
    match = null;
    foreach (user in this.Users()) {
        username = object_name(user);
        if (username == name && user.creator) return user; // exact
        if (username[0..len] == name && user.creator) {
            // Find the first match alphabetically
            if (!match || username < match) {
                match = username;
                found = user;
            }
        }
    }
    return found;
}

FindUser(name) {
    if (!name) return null;
    name = lower_case(name);
    len = length(name) - 1;
    found = null;
    match = null;
    foreach (user in this.Users()) {
        username = object_name(user);
        if (username == name) return user; // exact
        if (username[0..len] == name) {
            // Find the first match alphabetically
            if (!match || username < match) {
                match = username;
                found = user;
            }
        }
    }
    return found;
}

FindUser( sName )
{
    if( length( sName ) < 1 ) return null;

    sName = lower_case( sName );
    nLen = length( sName ) - 1;

    oFound = null;
    sMatch = null;
    foreach( oUser in this.Users() )
    {
        sUsername = object_name(oUser);
        if( sUsername == sName ) return oUser; // exact
        if( sUsername[0..nLen] == sName )
        {
            // Find the first match alphabetically
            if( !sMatch || sUsername < sMatch )
            {
                sMatch = sUsername;
                oFound = oUser;
            }
        }
    }

    return oFound;
}

GetNews( oCaller )
{
    sNews = "\n";

    if( oCaller.admin ) sNews += this.adminNews + "\n";
    if( oCaller.creator ) sNews += this.creatorNews + "\n";
    sNews += this.generalNews + "\n";

    return sNews;
}

GetUser( sName )
{
    sMatch = this.ExistsPartial( this.userList, sName );
    if( sMatch ) return this.userList[sMatch];
    return null;
}

Inform( sMessage )
{
    // Send an inform event to all on-line users
    this.Broadcast( "[" + sMessage + "]" );
}

Login( ptSocket )
{
    try
    {
        // Build the title screen
        sTitle = this.loginScreen;
        //sRandom = this.randomColours[random(sizeof(this.randomColours))];
        //sTitle = replace( sTitle, "#?", sRandom );

        // Add the online status bar
        nUsers = sizeof(this.Users());
        if( nUsers == 1 ) sInfo = "1 user online, up for ";
        else sInfo = to_string(nUsers) + " users online, up for ";
        //sInfo += format_time( uptime(), 2 ) + ".";
        sInfo += "some time.";
        //sTitle += center( sInfo, 70 ) + "\n\n";
        sTitle += "                  " + sInfo + "\n\n";

        sTitle = this.terminalDaemon.ResolveColour( sTitle + "#n", "ansi" );
        write_text( ptSocket, sTitle );

        repeat
        {
            // Display the login prompt
            write_text( ptSocket, "Who would you like to be today? " );

            sName = this.ReadLine( ptSocket );
            if( length( sName ) < 1 )
            {
                write_text( ptSocket, "\nYou must enter your name in order to log in.\n"
                  "Type 'q' to disconnect.\n" );
                continue;
            }

            sUsername = lower_case( sName );
            if( sUsername == "q" || sUsername == "quit" )
            {
                write_text( ptSocket, "\nDisconnecting...\n\n" );
                close( ptSocket );
                exit;
            }

            if( sUsername == "u" || sUsername == "w" )
            {
                // Display a list of users
                arUsers = this.Users();
                if( sizeof(arUsers ) == 0 )
                {
                    write_text( ptSocket, "\nThere are no players logged on.\n\n" );
                    continue;
                }
                else if( sizeof( arUsers ) == 1 )
                {
                    write_text( ptSocket, "\nThere is a single player on " + this.mudName + ":\n" );
                }
                else
                {
                    write_text( ptSocket, "\nThere are " + sizeof( arUsers ) + " players on " +
                      this.mudName + ":\n" );
                }
                write_text( ptSocket, this.UserList() + "\n\n" );
                continue;
            }

            if( length(sUsername) < 2 )
            {
                write_text( ptSocket, "\nUnknown login command. Enter 'q', 'u' or your player name.\n"
                  "Note that player names must be at least two letters long.\n\n" );
                  continue;
            }

            // Check if the user exists
            if( exists( this.userList, sUsername ) )
            {
                // Log in an existing user
                oUser = this.userList[sUsername];

                // Display the password screen
                sTitle = "\n" + this.passwdScreen + "\n";
                sTitle = this.terminalDaemon.ResolveColour( sTitle + "#n", "ansi" );
                write_text( ptSocket, sTitle );

                for( i = 0; i < 3; i += 1 )
                {
                    write_text( ptSocket, "Enter your password (press ENTER to cancel): " );
                    sPass = this.ReadLine( ptSocket );
                    if( sPass == "" )
                    {
                        if( i )
                        {
                            write_text( ptSocket, "\nWell, you had your chance. "
                              "Can't do any more than that.\n\n" );
                            close( ptSocket );
                            exit;
                        }
                        else
                        {
                            write_text( ptSocket, "\nLogin cancelled, about to redo from start.\n\n" );
                            i = -1;
                            break;
                        }
                    }

                    // Attempt to log the user in
                    if( oUser.Login( ptSocket, sPass ) )
                    {
                        // Add the user to the on-line list
                        this.LoginUser( oUser );
                        exit;
                    }

                    if( i < 2 )
                        write_text( ptSocket, "\nAha! That password is not correct. Pick again.\n\n" );
                }

                if( i == -1 ) continue;

                write_text( ptSocket, "\nOops, too many attempts - try again later.\n\n" );
                close( ptSocket );
                exit;
            }

            // User not found
            write_text( ptSocket, "\nYou do not appear to exist, sorry.\n\n" );
            sCreate = this.RequestOption( ptSocket, "Would you like to create '" +
              sUsername + "' (y/n)? ", [ "y", "n" ] );
            if( sCreate == "n" ) continue;

            this.CreateUser( ptSocket, sUsername );
        }
    }
    catch( err )
    {
        try
        {
            write_text( ptSocket, "\nOops, something has gone horribly wrong.\n" );
            write_text( ptSocket, this.DescribeError( err ) );
        }
        close( ptSocket );
    }
}

LoginUser( oUser )
{
    // Add the user to the on-line list
    remove( this.onLineList, oUser );
    this.onLineList += [ oUser ];
}

LogoutUser( oUser )
{
    // Remove the oCaller from the on-line users list
    // Don't send any messages from here!
    remove( this.onLineList, oUser );
}

Main(mudPort)
{
    console( "--> Starting listener for port " + mudPort );

    // Open a server port
    listenPort = listen( mudPort );

    console( "--> Accepting connections on port " + mudPort );

    // Listen for incoming connections forever
    repeat
    {
        try
        {
            // Accept a connection
            ptConn = accept( listenPort );
        }
        catch( err )
        {
            console( "--> Failed to accept connection on server port" );

            // Wait a bit, then restart the listener.
            sleep(5);
            thread this.Main( mudPort );
            exit;
        }

        try
        {
            // Spawn a login thread to handle the connection
            write_text( ptConn, version() + " running " + this.mudName + " " + this.mudVersion + "\n" );
            thread this.Login( ptConn );
        }
    }
}

RequestLine( ptSocket, sPrompt )
{
    repeat
    {
        // Display the prompt and read a line
        write_text( ptSocket, sPrompt );
        sLine = this.ReadLine( ptSocket );
        if( sLine == "" ) continue;
        return sLine;
    }
}

RequestOption( ptSocket, sPrompt, arOptions, sError )
{
    repeat
    {
        // Display the prompt and read a line
        write_text( ptSocket, sPrompt );
        sLine = lower_case( this.ReadLine( ptSocket ) );
        if( sLine == "" ) continue;

        // Check if the input matches an option
        foreach( sOption in arOptions )
        {
            if( sLine == sOption )
            {
                write_text( ptSocket, "\n");
                return sOption;
            }
        }

        // Display the error
        if( sError ) write_text( ptSocket, sError );
    }
}

Reset()
{
    // Restart the main thread if stopped
    if( !this.mainThread && this.mudPort ) {
        this.mainThread = thread this.Main(this.mudPort);
    }
    if( !this.mainThreadAlt && this.mudPortAlt ) {
        this.mainThreadAlt = thread this.MainAlt(this.mudPortAlt);
    }
}

Startup()
{
    // Clear the on-line user list
    this.onLineList = [];

    // Kill the old main threads
    if( this.mainThread )
    {
        kill_thread( this.mainThread );
        this.mainThread = null;
    }
    if( this.mainThreadAlt )
    {
        kill_thread( this.mainThreadAlt );
        this.mainThreadAlt = null;
    }

    // Reset, which will start threads.
    this.Reset();
}

UserList()
{
    // Build a comma-seperated list of users on-line
    arUsers = this.Users();
    nLast = sizeof(arUsers) - 1;
    if( nLast < 0 ) return "(none)";
    if( nLast == 0 ) return arUsers[0].Name() + ".";
    result = arUsers[0].Name();
    for( i = 1; i < nLast; i += 1 )
    {
        result += ", " + arUsers[i].Name();
    }
    return result + " and " + arUsers[nLast].Name() + ".";
}

UserLocations()
{
    // Return a list of user locations without duplicates
    result = new mapping;
    foreach( user in this.Users() ) result[user.location] = 1;
    return keys( result );
}

Users()
{
    remove( this.onLineList, null );

    // Remove any disconnected users from the list
    foreach( oUser in this.onLineList )
    {
        if( !oUser.IsConnected() ) this.LogoutUser( oUser );
    }

    return this.onLineList;
}

[$.lib.daemon.npcact]
-> $.lib.basic.daemon
actionChance = 50
baseTime = 30
mainThread = null
npcs = [$.mud.npc.phoenix#49, $.mud.npc.receptionist#84, $.mud.npc.org#104, $.home.raide.wraven#112, $.area.elv.arian.oak.liv.biddy#138, $.area.elv.arian.oak.liv.genshop#149, $.area.carnival.sw.liv.alchem#151, $.area.carnival.sw.liv.potion#152, $.area.carnival.sw.liv.healer#153, $.area.carnival.sw.liv.charms#154, $.area.carnival.sw.liv.scrollmage#155, $.area.carnival.sw.liv.mairtin#156, $.area.carnival.sw.liv.carla#157, $.area.carnival.sw.liv.sean#158, $.area.carnival.sw.liv.buttons#159, $.home.llhorian.liv.eep#160, $.mud.npc.test#198, $.mud.npc.kassis_sentry#200, $.mud.npc.gaspode#201, $.mud.npc.kitten#202, $.mud.npc.archchancellor#203]
randomTime = 30
xnpcs = []

Main()
{
    repeat
    {
        // Wait a while between actions
        sleep( this.baseTime + random( this.randomTime ) );

        // Perform an action on each npc based on chance
        nChance = this.actionChance;
        remove( this.npcs, null );
        foreach( oNPC in this.npcs )
        {
            if( random( 100 ) < nChance ) oNPC.RandomAction();
        }
    }
}

RegisterNpc( oNPC )
{
    this.npcs += [ oNPC ];
}

UnRegisterNpc( oNPC )
{
    this.npcs -= [ oNPC ];
}

[$.lib.daemon.roomact]
-> $.lib.basic.daemon
baseTime = 60
mainThread = null
randomTime = 60

Main()
{
    repeat
    {
        // Wait a while between actions
        sleep( this.baseTime + random( this.randomTime ) );

        // Perform an action in each occupied room
        foreach( oRoom in this.loginDaemon.UserLocations() )
        {
            if( objectp(oRoom) ) oRoom.RandomAction();
        }
    }
}

[$.lib.daemon.social]
-> $.lib.basic.daemon

[$.lib.daemon.social.socs]

[$.lib.daemon.social.socs.fong]
defult = "$a look*s around for someone to fong."
item = "$a look*s around for someone to fong with $ap $i."
target = "$a threaten*s to fong $t."
targetItem = "$a look*s around for someone to fong with $ap $i."

[$.lib.daemon.terminal]
-> $.lib.basic.daemon

[$.lib.daemon.terminal]
ResolveColour( sText, sTerm )
{
    mpEscSeq = this.map[sTerm] || this.map["default"];

    sEmpty = "";
    sResult = "";
    sPrefix = "#";
    nStart = 0;

    repeat
    {
        nOfs = search( sText, sPrefix, nStart );
        if( nOfs >= 0 )
        {
            // Keep the bit between the start and the prefix
            if( nOfs ) sResult += sText[nStart..nOfs-1];

            // Extract the colour character
            nOfs += 1;
            sChar = sText[nOfs..nOfs];
            nStart = nOfs + 1;

            // Add the colour escape sequence into the string
            sResult += mpEscSeq[sChar] || sEmpty;
        }
        else return sResult + sText[nStart..-1];
    }
}

[$.lib.daemon.terminal.map]
ansi = {"#" : "#", "N" : "\r\n", "B" : "\x1b[1m\x1b[34m", "C" : "\x1b[1m\x1b[36m", "E" : "\x07", "F" : "\x1b[5m", "G" : "\x1b[1m\x1b[32m", "K" : "\x1b[1m\x1b[30m", "L" : "\x1b[1m", "M" : "\x1b[1m\x1b[35m", "O" : "\x1b[1m\x1b[33m", "R" : "\x1b[1m\x1b[31m", "W" : "\x1b[1m\x1b[37m", "Y" : "\x1b[1m\x1b[33m", "b" : "\x1b[0m\x1b[34m", "c" : "\x1b[0m\x1b[36m", "g" : "\x1b[0m\x1b[32m", "k" : "\x1b[0m\x1b[30m", "m" : "\x1b[0m\x1b[35m", "n" : "\x1b[0m\x1b[37m", "o" : "\x1b[0m\x1b[33m", "r" : "\x1b[0m\x1b[31m", "w" : "\x1b[0m\x1b[37m", "y" : "\x1b[0m\x1b[33m"}
default = {"#" : "#", "N" : "\r\n"}
html = {"#" : "#", "N" : "<br>\n", "B" : "<font color='blue'><b>", "C" : "<font color='cyan'><b>", "F" : "", "G" : "<font color='green'><b>", "K" : "<font color='black'><b>", "L" : "<b>", "M" : "<font color='magenta'><b>", "O" : "<font color='orange'><b>", "R" : "<font color='red'><b>", "W" : "<font color='white'><b>", "Y" : "<font color='yellow'><b>", "b" : "<font color='blue'>", "c" : "<font color='cyan'>", "g" : "<font color='green'>", "k" : "<font color='black'>", "m" : "<font color='magent'>", "n" : "</font>", "o" : "<font color='orange'>", "r" : "<font color='red'>", "w" : "<font color='white'>", "y" : "<font color='yellow'>"}

[$.lib.daemon.worldsys]
-> $.lib.basic.daemon
day = 1
dayName = "Sunday"
hour = 13
mainThread = null
month = 1
monthName = $.mud.moon.ruis
week = 2
year = 10929
hourTicks = 60
dayLine = ""
sky1 = ""
sky2 = ""

dayNames = [
  "Sunday",
  "Moonday",
  "Lairday",
  "Wilfday",
  "Treeday",
  "Faeday",
  "Skyday"
]

dayLabels = [
"#w---{#mSunday#w}---",
"#w---{#mMoonday#w}--#n",
"#w---{#mLairday#w}--#n",
"#w---{#mWilfday#w}--#n",
"#w---{#mTreeday#w}--#n",
"#w---{#mFaeday#w}---#n",
"#w---{#mSkyday#w}---#n"
]

[$.lib.daemon.worldsys]
Main()
{
  repeat
  {
    // tick the hour over

    sleep( this.hourTicks );

    this.hour += 1;

    if( this.hour >= 25 ) {
      this.hour = 1;
      this.day += 1;

      if( this.day >= 8 )
      {
        this.day = 1;
        this.week += 1;
      }

      if( this.week >= 5 )
      {
        this.week = 1;
        this.month += 1;

        if( this.month >= 14 )
        {
          this.month = 1;
          this.year += 1;
        }
      }
    }

    this.dayName = this.dayNames[this.day-1];
    this.dayLine = this.dayLabels[this.day-1];
    this.UpdateSky();
  }
}


meh()
{
         switch( this.month )
         {
           case 1:
            this.monthName = $.mud.moon.beth;
           break;
           case 2:
            this.monthName = $.mud.moon.luis;
           break;
           case 3:
            this.monthName = $.mud.moon.nion;
           break;
           case 4:
            this.monthName = $.mud.moon.fearn;
           break;
           case 5:
            this.monthName = $.mud.moon.saille;
           break;
           case 6:
            this.monthName = $.mud.moon.uath;
           break;
           case 7:
            this.monthName = $.mud.moon.duir;
           break;
           case 8:
            this.monthName = $.mud.moon.tinne;
           break;
           case 9:
            this.monthName = $.mud.moon.coll;
           break;
           case 10:
            this.monthName = $.mud.moon.muin;
           break;
           case 11:
            this.monthName = $.mud.moon.gort;
           break;
           case 12:
            this.monthName = $.mud.moon.ngetal;
           break;
           case 13:
            this.monthName = $.mud.moon.ruis;
           break;
         }
}


#W   ~~~    ~~   ~~~  ~
#W~~   ~~~    ~~   ~~~

#y   ~~~    ~~   ~~~  ~
#y~~   ~~~    ~~   ~~~

#w .     .   .    .
#w    .    .   .     .

#r   ~~~    ~~   ~~~  ~
#r~~   ~~~    ~~   ~~~



[$.lib.daemon.worldsys]
skyList = [
["#w.    . #W)#w   .  #n",
 "#w  .    .     .#n"],

["#w   .  #W)#w .   . #n",
 "#w .    .   .   #n"],

["#w .   #W)#w .   .  #n",
 "#w    .    .   .#n"],

["#w    #W)#w  .    . #n",
 "#w .    .   .   #n"],

["#W  )#w  .    .   #n",
 "#w  .    .     .#n"],

["#K     .    .   #n",
 "#w )#K .   .    . #n"],

["#y  ~      ~    #n",
 "#y     ~     ~ o#n"],

["#W ~      ~   #Yo #n",
 "#W    ~     ~   #n"],

["#W~      ~   #Yo  #n",
 "#W   ~     ~    #n"],

["#W      ~   #Yo#W  ~#n",
 "#W  ~     ~     #n"],

["#W     ~   #Yo#W  ~ #n",
 "#W ~     ~      #n"],

["#W    ~   #Yo#W  ~  #n",
 "#W~     ~       #n"],

["#W   ~   #Yo#W  ~   #n",
 "#W     ~       ~#n"],

["#W  ~   #Yo#W  ~    #n",
 "#W    ~       ~ #n"],

["#W ~   #Yo#W  ~     #n",
 "#W   ~       ~  #n"],

["#W~   #Yo#W  ~      #n",
 "#W  ~       ~   #n"],

["#W   #Yo#W  ~      ~#n",
 "#W ~       ~    #n"],

["#W  #Yo#W  ~      ~ #n",
 "#W~       ~     #n"],

["#Y o#W  ~     ~   #n",
 "#W       ~     ~#n"],

["#y   ~     ~    #n",
 "#Yo#y     ~     ~ #n"],

["#r  ~     ~     #n",
 "#yo#r    ~     ~  #n"],

["#K   .    .  #w)  #n",
 "#K .   .      . #n"],

["#w  .    .  #W)   #n",
"#w    .    .  . #n"],

["#w .   .  #W)#w    .#n",
"#w   .    .  .  #n"]
]

[$.lib.daemon.worldsys]
UpdateSky()
{
  hour = this.hour;
  if (hour < 1) hour = 1;
  else if (hour > 24) hour = 24;

  day = this.skyList[hour-1];
  this.sky1 = day[0];
  this.sky2 = day[1];
}


Foo()
{
  if (hour <= 6) {
    // 1-6 left half
    night = 1;
    sunPos = 9 - (hour - 1);
    sun = "#K)#W";
  } else if (hour >= 19) {
    // 19-24 right half
    night = 1;
    sunPos = 15 - (hour - 19);
    sun = "#K)#W";
  } else {
    // 7-18 full width
    night = 0;
    sunPos = 15 - (hour - 7);
    sun = "#Y@#W";
  }

  if (night) {
    line1 = "#W  .    .   .  #n";
    line2 = "#W.   .    .  . #n";
  } else {
    line1 = "#W  ~      ~    #n";
    line2 = "#W     ~    ~   #n";
  }

  line1 = line1[..sunPos-1] + sun + line1[sunPos+1..];

  this.sky1 = line1;
  this.sky2 = line2;

  return sunPos;
}

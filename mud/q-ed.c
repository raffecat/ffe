[$.lib.user.user]

EditBufferSize( sBuffer )
{
    if( !this.editBuffers || !(arInfo = this.editBuffers[sBuffer]) ) return 0;
    return length(arInfo[0]);
}

EditCloseBuffer( sBuffer )
{
    if( !this.editBuffers || !(arInfo = this.editBuffers[sBuffer]) ) return 0;

    // Delete the buffer
    remove( this.editBuffers, sBuffer );
    return 1;
}

EditCommand( sBuffer, sCmd, oCaller )
{
    // Check if the buffer is open
    if( !this.editBuffers || !(arInfo = this.editBuffers[sBuffer]) )
        return "Buffer is not open.";

    if( length(sCmd) < 1 ) return 1;

    nBufferLen = sizeof(arInfo[0]);
    if( nBufferLen < 1 ) nBufferLen = 1;

    // Check for a line number in the command
    nFirst = to_int(sCmd);
    if( nFirst != null )
    {
        nPos = length(to_string(nFirst));
        if( nFirst < 1 ) nFirst = 1; else if( nFirst > nBufferLen ) nFirst = nBufferLen;
    }
    else nPos = 0;

    // Check for a line range in the command
    if( sCmd[nPos..nPos] == "," )
    {
        if( nFirst == null ) nFirst = 1; // implicit start line
        nPos += 1;
        if( sCmd[nPos..nPos] == "*" ) { nPos += 1; nLast = null; }
        else nLast = to_int(sCmd[nPos..]);
        if( nLast != null )
        {
            nPos += length(to_string(nLast));
            if( nLast < nFirst ) nLast = nFirst; else if( nLast > nBufferLen ) nLast = nBufferLen;
        }
        else nLast = nBufferLen;
    }
    else
    {
        if( nFirst == null ) nFirst = arInfo[4]; // current line
        if( nFirst > nBufferLen ) nFirst = nBufferLen;
        nLast = nFirst;
    }

    sOp = sCmd[nPos..nPos];
    if( length(sOp) < 1 ) sOp = "c";
    nPos += 1;

    if( sCmd[nPos..nPos] == " " ) sArg = sCmd[nPos+1..];
    else sArg = null;

//oCaller.RawMessage( "-> " + nFirst + ", " + nLast + ", " + sOp + ", '" + to_string(sArg) + "'\n" );

    switch( sOp )
    {
        case "q":
            // Quit with no changes

            if( this.EditIsDirty(sBuffer) )
                return "Buffer has been modified. Use 'Q' to lose changes.";

            if( this.EditCloseBuffer( sBuffer ) )
                return "Closing editor (no changes).";

            return "Failed to close edit buffer.";

        case "Q":
            // Quit and lose changes

            if( this.EditCloseBuffer( sBuffer ) )
                return "Closing editor (any changes have been lost).";

            return "Failed to close edit buffer.";

        case "x":
            // Save or Compile and quit

            if( this.EditIsDirty(sBuffer) )
            {
                if( this.EditIsFunction( sBuffer ) )
                {
                    mResult = this.EditCompile( sBuffer, null, oCaller );
                    if( stringp(mResult) ) return mResult;
                    oCaller.MessageNoWrap( "Compiled." );
                }
                else
                {
                    mResult = this.EditSave( sBuffer, null, oCaller );
                    if( stringp(mResult) ) return mResult;
                    oCaller.MessageNoWrap( "Saved." );
                }

            }

            this.EditCloseBuffer( sBuffer );
            return 1;

        case "s":
            // Save string buffer

            if( !this.EditIsFunction( sBuffer ) )
            {
                mResult = this.EditSave( sBuffer, sArg, oCaller );
                if( stringp(mResult) ) return mResult;

                return "Saved.";
            }

            return "Editing a function; use 'S' to save source.";

        case "S":
            // Save string or function buffer as a string

            mResult = this.EditSave( sBuffer, sArg, oCaller );
            if( stringp(mResult) ) return mResult;

            return "Saved.";

        case "C":
            // Compile buffer to a function and save

            mResult = this.EditCompile( sBuffer, sArg, oCaller );
            if( stringp(mResult) ) return mResult;

            return "Compiled.";

        case "c":
            // Change lines in the buffer until "."

            if( !this.EditRemoveLines( sBuffer, nFirst, nLast ) )
                return "Failed to change lines.";

            nLine = nFirst;
            repeat
            {
                sArg = oCaller.ReadLine( nLine + "*" );
                if( sArg == "." ) break;
                if( !this.EditInsertLine( sBuffer, nLine, sArg ) )
                    return "Failed to insert line.";
                nLine += 1;
            }

            arInfo[4] = nLine;
            return 1;

        case "a":
            // Append lines to the buffer until "."

            nLine = nFirst + 1;
            repeat
            {
                sArg = oCaller.ReadLine( nLine + "*" );
                if( sArg == "." ) break;
                if( !this.EditInsertLine( sBuffer, nLine, sArg ) )
                    return "Failed to insert line.";
                nLine += 1;
            }

            arInfo[4] = nLine;
            return 1;

        case "i":
            // Insert lines into the buffer until "."

            nLine = nFirst;
            repeat
            {
                sArg = oCaller.ReadLine( nLine + "*" );
                if( sArg == "." ) break;
                if( !this.EditInsertLine( sBuffer, nLine, sArg ) )
                    return "Failed to insert line.";
                nLine += 1;
            }

            arInfo[4] = nLine;
            return 1;

        case "d":
            // Delete lines from the buffer
            if( !this.EditRemoveLines( sBuffer, nFirst, nLast ) )
                return "Failed to remove lines.";

            arInfo[4] = nFirst;
            return 1;

        case "p":
            // Display lines of the buffer

            arInfo[4] = nLast;
            this.EditDisplay( sBuffer, nFirst, nLast, oCaller );
            return 1;

        case "z":
            // Display 20 lines of the buffer

            nLast = nFirst + 20;
            if( nLast > nBufferLen ) nLast = nBufferLen;
            arInfo[4] = nLast;
            this.EditDisplay( sBuffer, nFirst, nLast, oCaller );
            return 1;

        case "Z":
            // Display 40 lines of the buffer

            nLast = nFirst + 40;
            if( nLast > nBufferLen ) nLast = nBufferLen;
            arInfo[4] = nLast;
            this.EditDisplay( sBuffer, nFirst, nLast, oCaller );
            return 1;
    }

    return 0;
}

[$.lib.user.user]
EditCompile( sBuffer, sNewPath, oCaller )
{
    if( !this.editBuffers || !(arInfo = this.editBuffers[sBuffer]) )
        return "Buffer is not open.";

    // Check if the buffer has a path
    if (stringp(sNewPath)) arInfo[1] = sNewPath;
    sPath = arInfo[1]; // path
    if (sPath) {
        // Find the target object and name
        arTarget = this.FindPathName( sPath, oCaller.cwd );
        if( !arTarget ) return "Not found: " + sPath;
        oTarget = arTarget[0];
        sName = arTarget[1];

        // Attempt to compile the buffer into a function
        sValue = join(arInfo[0], "\n");
        result = parse(sValue, 1);

        // Display any errors
        if (stringp(result)) oCaller.MessageNoWrap("#C"+replace(result,"#","##"));

        // If compiled, save the result
        if (functionp(result)) {
            oTarget[sName] = result;
            arInfo[3] = 0; // dirty flag
            return 1;
        }

        return "Function did not compile.";
    }

    return "No path has been specified.";
}

EditDisplay( sBuffer, nFirst, nLast, oCaller )
{
    // Check if the buffer is open
    if( !this.editBuffers || !(arInfo = this.editBuffers[sBuffer]) )
        return "Buffer is not open.";

    mText = arInfo[0];
    nLen = sizeof(mText);
    if( nLen )
    {
        if( nLast > nLen ) nLast = nLen;
        nPad = length(to_string(nLast)) + 1;
        nCurrent = arInfo[4];
        for( nLine = nFirst; nLine <= nLast; nLine += 1 )
        {
            if( nLine == nCurrent ) sNum = nLine + "*"; else sNum = nLine + " ";
            oCaller.RawMessage( this.RepeatString( " ", nPad - length(sNum) ) +
                sNum + mText[nLine - 1] + "\n" );
        }
    }
    else oCaller.RawMessage( "Buffer is empty.\n" );

    return 1;
}

EditGetPath( sBuffer )
{
    if( !this.editBuffers || !(arInfo = this.editBuffers[sBuffer]) ) return null;
    return arInfo[1];
}

EditInsertLine( sBuffer, nLine, sText )
{
    // Check if the buffer is open
    if( !this.editBuffers || !(arInfo = this.editBuffers[sBuffer]) ) return 0;

    nLine -= 1; // line 1 is index 0

    // Insert the new line
    nLen = length(arInfo[0]);
    if( nLine > 0 ) arResult = arInfo[0][0..nLine - 1] + [ sText ];
    else arResult = [ sText ];
    if( nLine < nLen ) arResult += arInfo[0][nLine..nLen];

    arInfo[0] = arResult;
    arInfo[3] = 1; // dirty flag

    if( nLine < 0 ) nLine = 0; else if( nLine > nLen ) nLine = nLen;
    return nLine + 1;
}

EditIsDirty( sBuffer )
{
    if( !this.editBuffers || !(arInfo = this.editBuffers[sBuffer]) ) return 0;
    return arInfo[3];
}

EditIsFunction( sBuffer )
{
    if( !this.editBuffers || !(arInfo = this.editBuffers[sBuffer]) ) return 0;
    return arInfo[2];
}

EditIsOpen( sBuffer )
{
    if( !this.editBuffers || !(arInfo = this.editBuffers[sBuffer]) ) return 0;
    return 1;
}

EditOpenBuffer( sBuffer, mText, sPath, bIsFunction )
{
    if( !mText ) mText = [];
    if( !bIsFunction ) bIsFunction = 0;

    // Close the editor if currently open (lose changes)
    this.EditCloseBuffer( sBuffer );

    // Validate the text buffer
    if( stringp(mText) )
    {
        // Split the text into individual lines
        mText = split( replace( mText, "\r\n", "\n" ), "\n" );
    }
    else if( !arrayp(mText) ) return "Text is not a string or array.";

    bDirty = 0;

    // Create the edit buffer
    if( !this.editBuffers ) this.editBuffers = new mapping;
    this.editBuffers[sBuffer] = [ mText, sPath, bIsFunction, bDirty, 1 ];

    return 1;
}

EditRemoveLines( sBuffer, nFirst, nLast )
{
    // Check if the buffer is open
    if( !this.editBuffers || !(arInfo = this.editBuffers[sBuffer]) ) return 0;

    nFirst -= 1; // line 1 is index 0
    nLast -= 1;

    // Remove the specified line
    nLen = length(arInfo[0]);
    if( nFirst > 0 )
    {
        arResult = arInfo[0][0..nFirst - 1];
        if( nLast < nLen - 1 ) arResult += arInfo[0][nLast + 1..nLen];
    }
    else arResult = arInfo[0][nLast + 1..];

    arInfo[0] = arResult;
    arInfo[3] = 1; // dirty flag

    return 1;
}

EditSave( sBuffer, sNewPath, oCaller )
{
    if( !this.editBuffers || !(arInfo = this.editBuffers[sBuffer]) )
        return "Buffer is not open.";

    // Check if the buffer has a path
    if( stringp(sNewPath) ) arInfo[1] = sNewPath;
    sPath = arInfo[1]; // path
    if( length(sPath) )
    {
        // Find the target object and name
        arTarget = this.FindPathName( sPath, oCaller.cwd );
        if( !arTarget ) return "Not found: " + sPath;
        oTarget = arTarget[0];
        sName = arTarget[1];

        sValue = join( arInfo[0], "\n" );
        oTarget[sName] = sValue;

        arInfo[3] = 0; // dirty flag

        return 1;
    }

    return "No path has been specified.";
}

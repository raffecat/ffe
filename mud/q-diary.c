[$.mud.item.diary]
-> $.lib.basic.item

id = "diary"
name = "small silver-bound diary"
isOpen = 0
board = null
boardList = $.save.board
desc = "A small silver-bound diary with '#YQuintiqua#n' written across the front in gold lettering. A number of coloured leather bookmarks dangle in disarray from the bottom of the diary."

Browse( sItem, oCaller )
{
    foreach( sBoard in this.boardList )
    {
        oBoard = this.boardList[sBoard];
        foreach( arNote in oBoard.notes )
        {
            bFound = 0;
            foreach( oUser in arNote[4] )
            {
                if( oUser == oCaller )
                {
                    bFound = 1;
                    break;
                }
            }
            if( !bFound )
            {
                // Select this board - unread news items
                if( this.Open( sBoard, oCaller ) )
                    return this.Read( "", oCaller );
            }
        }
    }

    return "There are no new posts in " + this.TheName() + ".";
}

Close( sItem, oCaller )
{
    if( this.isOpen != 1 ) return capitalise(this.TheName()) + " is already closed.";
    this.isOpen = 0;
    return 1;
}

Describe()
{
    sDesc = this.desc + "\n\n" + capitalise(this.TheName()) + " is ";
    if( this.isOpen == 1 ) sDesc += "open to the bookmark labelled '" + this.board.name + "#n'.";
    else sDesc += "closed.";

    sDesc += "\n\nBookmarks: ";
    bFirst = 1;
    foreach( sBoard in this.boardList )
    {
        if( bFirst ) bFirst = 0; else sDesc += ", ";
        sDesc += this.boardList[sBoard].name + "#n";
    }

    return sDesc + ".";
}

Open( sItem )
{
    if( sItem )
    {
        sItem = lower_case(sItem);
        sBoard = this.ExistsPartial( this.boardList, sItem );
        if( sBoard )
        {
            this.board = this.boardList[sBoard];
            this.isOpen = 1;
        }
        else
        {
            return "There is no bookmark labelled '" + sItem + "' in " + this.TheName() + ".";
        }
    }
    else
    {
        if( this.isOpen == 1 ) return capitalise(this.TheName()) + " is already open.";
        this.isOpen = 1;
    }

    return "You open " + this.TheName() + " to the bookmark labelled '" + this.board.name + "#n'.";
}

PostIn( sItem, oCaller )
{
    if( this.isOpen != 1 ) return capitalise(this.TheName()) + " is closed.";
    if( this.board.info ) return "You cannot post in this section of " + this.TheName + ".";

    sSubject = oCaller.ReadLine( "Subject: " );
    if( !sSubject ) return "Post cancelled.";

    sBody = this.SimpleEdit( oCaller, "Editing New Post: " + sSubject, null, 0 );
    if( !sBody ) return "Post cancelled.";

    arRead = [];
    arNew = [ time(), oCaller, sSubject, sBody, arRead ];
    this.board.notes += [ arNew ];

    return 1;
}

[$.mud.item.diary]
Read( sItem, oCaller )
{
    if( this.isOpen != 1 ) return capitalise(this.TheName()) + " is closed.";

    if( length(sItem) )
    {
        nPost = to_int(sItem);
        if( nPost > 0 && nPost <= length(this.board.notes) )
        {
            arNote = this.board.notes[nPost - 1];

            user = arNote[1];
            if (user) username = user.name; else username = "(deleted)";

            sResult = "Note: " + nPost + "  Date: " + ctime(arNote[0]) +
            "  Author: " + username + "\nSubject: " + arNote[2] +
            "\n\n" + arNote[3] + "\n";

            bFound = 0;
            foreach( oUser in arNote[4] )
            {
                if( oUser == oCaller )
                {
                    bFound = 1;
                    break;
                }
            }
            if( !bFound ) arNote[4] += [ oCaller ];
        }
        else return "There is no such note in " + this.TheName() + ".";
    }
    else
    {
        sResult =
          "#o==============================================================================\n"
          "#rBookmark:#n " + this.board.name + "#n\n"
          "#o==============================================================================#n\n\n";

        if( this.board.info )
        {
            sResult += this.board.info + "#n\n";
        }
        else if( length(this.board.notes) )
        {
            nIndex = 1;
            foreach( arNote in this.board.notes )
            {
                sResult += "[" + nIndex + "] #Y";
                bFound = 0;
                foreach( oUser in arNote[4] )
                {
                    if( oUser == oCaller )
                    {
                        bFound = 1;
                        break;
                    }
                }
                if( bFound ) sResult += " "; else sResult += "N";

                user = arNote[1];
                if (user) username = user.name; else username = "(deleted)";
                
                sResult += "#n " + this.PadString(username[0..14] + ":", 16) +
                  this.PadString("\"" + arNote[2][0..34] + "\"", 37) +
                  ctime(arNote[0])[0..15] + "\n";
                nIndex = nIndex + 1;
            }
        }
        else
        {
            sResult += "There are no notes in this section of the diary.\n";
        }
    }

    oCaller.Message( replace(sResult,"$","$$") + "\n" );

    return 1;
}

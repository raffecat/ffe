[$.lib.basic.area]
-> $.lib.global

avRain = 5
avTemp = 5
avWind = 5
desc = ""
map = []
name = "Somewhere"
origin = null
xMax = 0
xMin = 0
yMax = 0
yMin = 0
zMax = 0
zMin = 0

FixRoom( oRoom )
{
    if( exists( oRoom, "level" ) ) remove( oRoom, "level" ); // temp cleanup
    if( stringp(oRoom.class) )
    {
        // another hack fix
        oRoom.class = find_object( "$.mud.class.room." + oRoom.class );
    }

    if( exists( oRoom, "daylight" ) )
    {
        oRoom.light = to_int( oRoom.daylight / 10 );
        remove( oRoom, "daylight" );
        remove( oRoom, "ambient" );
        remove( oRoom, "nightambient" );
    }
    else if( realp( oRoom.light ) ) oRoom.light = to_int( oRoom.light );

    nExit = -1;
    foreach( arExit in oRoom.exits )
    {
        nExit += 1;

        if( stringp(arExit[2]) )
        {
            arExit[2] = find_object( "$.mud.class.exit." + arExit[2] );
            oRoom.exits[nExit] = arExit;
        }
    }
}


[$.lib.basic.area]
SetRoomAt(xpos, ypos, zpos, newRoom) {
    if (xpos < -100 || xpos > 100) return;
    if (ypos < -100 || ypos > 100) return;
    // add map rows at the top or bottom.
    while (ypos < this.yMin) {
        this.map = [ [] ] + this.map;
        this.yMin -= 1;
    }
    // add columns to the left side.
    while (xpos < this.xMin) {
        for( i = 0; i < length(this.map); i += 1 ) {
            this.map[i] = [ null ] + this.map[i];
        }
        this.xMin -= 1;
    }
    rowNum = ypos - this.yMin;
    colNum = xpos - this.xMin;
    // might need to add rows to hold ypos.
    if (ypos > this.yMax) this.yMax = ypos;
    while (length(this.map) <= rowNum) {
        this.map = this.map + [ [] ];
    }
    // might need to add cols to reach xpos.
    if (xpos > this.xMax) this.xMax = xpos;
    while (length(this.map[rowNum]) <= colNum) {
        this.map[rowNum] = this.map[rowNum] + [ null ];
    }
    roomOrList = this.map[rowNum][colNum];
    if (!roomOrList) this.map[rowNum][colNum] = newRoom;
    else if (arrayp(roomOrList)) {
        // do not add the same room twice.
        found = 0;
        foreach (room in roomOrList) {
            if (room == newRoom) { found=1; break; }
        }
        if (!found) {
            this.map[rowNum][colNum] = roomOrList + [ newRoom ];
        }
    } else if (objectp(roomOrList)) {
        // even if it's the only room.
        if (roomOrList != newRoom) {
            this.map[rowNum][colNum] = [ roomOrList, newRoom ];
        }
    }
    // clear cached maps in all rooms.
    foreach (row in this.map) {
        foreach (col in row) {
            if (objectp(col)) col.minimap = null;
            else if (arrayp(col)) {
                foreach (room in col) room.minimap = null;
            }
        }
    }
}


[$.lib.basic.area]
RoomAt(xpos, ypos, zpos) {
    rowNum = ypos - this.yMin;
    colNum = xpos - this.xMin;
    if (rowNum < 0) return null; // off the top edge.
    if (colNum < 0) return null; // off the left edge.
    mapRow = this.map[rowNum];
    if (mapRow) {
        room = mapRow[colNum];
        if (objectp(room) && room.zpos == zpos) {
            return room;
        }
        if (arrayp(room)) {
            for (i=0; i<length(room); i+=1) {
                obj = room[i];
                if (objectp(obj) && obj.zpos == zpos) {
                    return obj;
                }
            }
        }
    }
    return null;
}

-- script
set $row = self.map at ($ypos + self.yMin)
set $col = $row at ($xpos + self.xMin)
ret $room = $col | has zpos = $zpos


[$.lib.basic.area]
RenderMap( oHere, nWidth, nHeight, oCaller, x, y )
{
    // Build a buffer to hold the map
    arMap = [];
    for( i = 0; i < nHeight; i += 1 ) {
        arMap += [ this.RepeatString( " ", nWidth ) ];
    }

    nPlotLeft = nPlotY = 0;

    // Determine the top-left coordinates in the area
    if (x != null) {
        nLeft = this.xMin - to_int( nWidth / 4 );
        nTop = this.yMin - to_int( nHeight / 4 );
        nRight = nLeft + to_int( nWidth / 2 ) + 1;
        nBottom = nTop + to_int( nHeight / 2 ) + 1;
    } else {
        nLeft = oHere.xpos - to_int( nWidth / 4 );
        nTop = oHere.ypos - to_int( nHeight / 4 );
        nRight = nLeft + to_int( nWidth / 2 ) + 1;
        nBottom = nTop + to_int( nHeight / 2 ) + 1;
    }
    nAlt = oHere.zpos;

    //oCaller.RawMessage( "Coords " + nLeft + ", " + nTop + " - " + nRight + ", " + nBottom + "\n" );

    // Determine the top-left indices into the map data
    nPosX = nLeft - this.xMin;
    nPosY = nTop - this.yMin;
    if( nPosX < 0 ) { nPlotLeft = -nPosX * 2; nPosX = 0; }
    if( nPosY < 0 ) { nPlotY = -nPosY * 2; nPosY = 0; }

    nMapWidth = this.xMax - this.xMin + 1;
    nMapHeight = this.yMax - this.yMin + 1;

    // Determine the bottom-right indices into the map data
    nEndX = nRight - this.xMin;
    nEndY = nBottom - this.yMin;
    if( nEndX > nMapWidth ) nEndX = nMapWidth;
    if( nEndY > nMapHeight ) nEndY = nMapHeight;

    //oCaller.RawMessage( "Indices " + nPosX + ", " + nPosY + " - " + nEndX + ", " + nEndY + "\n" );

    pMap = this.map;
    for( nRow = nPosY; nRow < nEndY; nRow += 1 ) {
        nPlotX = nPlotLeft;
        for( nCol = nPosX; nCol < nEndX; nCol += 1 ) {
            arRow = pMap[nRow];
            if( length(arRow) > nCol ) oRoom = pMap[nRow][nCol]; else oRoom = null;

            bTrace = ( oRoom != null );
            bHere = 0;
            bClash = 0;
            if( arrayp(oRoom) ) {
                // Find the room at this Z coordinate
                oFound = null;
                foreach( oObj in oRoom ) {
                    if( oObj.zpos == nAlt ) {
                        if( oFound ) bClash = 1;
                        if( oObj == oHere ) bHere = 1;
                        oFound = oObj;
                    }
                }
                oRoom = oFound;
            } else if( oRoom ) {
                // Check if visible on this map
                if( oRoom == oHere ) bHere = 1;
                if( oRoom.zpos != nAlt ) oRoom = null;
            }

            if( oRoom ) {
                sRoom = "o"; // room

                foreach( arExit in oRoom.exits ) {
                    if (dir[2].hidden) continue; // exit class hidden.
                    switch( arExit[0] ) {
                        case "north": if( nPlotY ) arMap[nPlotY - 1] = this.AssignChar( arMap[nPlotY - 1], nPlotX, "|" ); break;
                        case "south": if( nPlotY < nHeight - 1 ) arMap[nPlotY + 1] = this.AssignChar( arMap[nPlotY + 1], nPlotX, "|" ); break;
                        case "east": if( nPlotX < nWidth - 1 ) arMap[nPlotY] = this.AssignChar( arMap[nPlotY], nPlotX + 1, "-" ); break;
                        case "west": if( nPlotX ) arMap[nPlotY] = this.AssignChar( arMap[nPlotY], nPlotX - 1, "-" ); break;
                        case "northeast": if( nPlotY && nPlotX < nWidth - 1 ) arMap[nPlotY - 1] = this.AssignChar( arMap[nPlotY - 1], nPlotX + 1, "/" ); break;
                        case "northwest": if( nPlotY && nPlotX ) arMap[nPlotY - 1] = this.AssignChar( arMap[nPlotY - 1], nPlotX - 1, "\\" ); break;
                        case "southeast": if( nPlotY < nHeight - 1 && nPlotX < nWidth - 1 ) arMap[nPlotY + 1] = this.AssignChar( arMap[nPlotY + 1], nPlotX + 1, "\\" ); break;
                        case "southwest": if( nPlotY < nHeight - 1 && nPlotX ) arMap[nPlotY + 1] = this.AssignChar( arMap[nPlotY + 1], nPlotX - 1, "/" ); break;
                        case "up":  sRoom = "O"; break;
                        case "down": sRoom = "O"; break;
                    }
                }

                if( bHere ) sRoom = "*"; // self
                else if( bClash ) sRoom = "%"; // clash
                else if( arMap[nPlotY][nPlotX] != " " ) sRoom = "@"; // area-clash

                arMap[nPlotY] = this.AssignChar( arMap[nPlotY], nPlotX, sRoom );
            } else {
                if( bTrace && arMap[nPlotY][nPlotX] == " " )
                    arMap[nPlotY] = this.AssignChar( arMap[nPlotY], nPlotX, "." );
            }

            nPlotX += 2;
        }

        nPlotY += 2;
    }

    sMap = join( arMap, "\n" );
    sMap = replace( sMap, "o", "#go" );
    sMap = replace( sMap, "O", "#WO" );
    sMap = replace( sMap, "-", "#o-" );
    sMap = replace( sMap, "|", "#o|" );
    sMap = replace( sMap, "\\", "#o\\" );
    sMap = replace( sMap, "/", "#o/" );
    sMap = replace( sMap, "X", "#oX" );
    sMap = replace( sMap, "%", "#R%" );
    sMap = replace( sMap, "@", "#R@" );
    sMap = replace( sMap, "*", "#Y*" );
    sMap = replace( sMap, ".", "#K." );

    return sMap;
}

[$.lib.basic.area]
UpdateAreaMap( oCaller, bFixRooms )
{
    // Crawl the entire area starting from the origin room

    if( !this.origin ) return 0;

    mpDone = new mapping;
    pRooms = [ this.origin ];

    this.origin.area = this;
    this.origin.xpos = 0;
    this.origin.ypos = 0;
    this.origin.zpos = 0;

    pArea = [];
    nXMin = nYMin = nXMax = nYMax = nZMin = nZMax = 0;

    nTop = 0;
    nLeft = 0;
    pLeft = [];

    while( length(pRooms) )
    {
        pAdjacent = [];
        foreach( oRoom in pRooms )
        {
            if( exists( mpDone, oRoom ) ) continue;

            // Clear cached room maps
            oRoom.minimap = null;

            // Optionally repair broken rooms during mapping
            if( bFixRooms ) this.FixRoom( oRoom );

            // Get the current room position
            nPosX = oRoom.xpos;
            nPosY = oRoom.ypos;
            nPosZ = oRoom.zpos;

            // Update the bounding box
            if( nPosX < nXMin ) nXMin = nPosX;
            if( nPosY < nYMin ) nYMin = nPosY;
            if( nPosZ < nZMin ) nZMin = nPosZ;
            if( nPosX > nXMax ) nXMax = nPosX;
            if( nPosY > nYMax ) nYMax = nPosY;
            if( nPosZ > nZMax ) nZMax = nPosZ;

            // Make sure the Y index exists in the map
            while( nPosY < nTop )
            {
                pArea = [ [] ] + pArea;
                pLeft = [ 0 ] + pLeft;
                nTop -= 1;
            }
            nPosY -= nTop;
            while( length(pArea) <= nPosY )
            {
                pArea += [ [] ];
                pLeft += [ 0 ];
            }

            // Make sure the X index exists in the map
            while( nPosX < pLeft[nPosY] )
            {
                pArea[nPosY] = [ null ] + pArea[nPosY];
                pLeft[nPosY] -= 1;
            }
            nPosX -= pLeft[nPosY];
            if( pLeft[nPosY] < nLeft ) nLeft = pLeft[nPosY];
            while( length(pArea[nPosY]) <= nPosX )
            {
                pArea[nPosY] += [ null ];
            }

            // Add the room to this map location
            mExisting = pArea[nPosY][nPosX];
            if( arrayp(mExisting) )
            {
                pArea[nPosY][nPosX] += [ oRoom ];
                //oCaller.RawMessage( "Appended " + object_path(oRoom) + " at " + nPosX + ", " + nPosY + "\n" );
            }
            else if( objectp(mExisting) )
            {
                pArea[nPosY][nPosX] = [ mExisting, oRoom ];
                //oCaller.RawMessage( "Paired " + object_path(oRoom) + " with " + object_path(mExisting) + " at " + nPosX + ", " + nPosY + "\n" );
            }
            else
            {
                pArea[nPosY][nPosX] = oRoom;
                //oCaller.RawMessage( "Inserted " + object_path(oRoom) + " at " + nPosX + ", " + nPosY + "\n" );
            }

            //oCaller.RawMessage( "--> " + nPosX + ", " + nPosY + "\n" );

            // Flag the room as mapped
            mpDone[oRoom] = 1;

            // Enqueue the adjacent rooms
            nExit = -1;
            foreach( arExit in oRoom.exits )
            {
                nExit += 1;
                oAdj = arExit[1];
                if( !oAdj || exists( mpDone, oAdj ) ) continue;

                if( !oAdj.area ) oAdj.area = this;
                else if( oAdj.area != this ) continue;

                // Get the current room position
                nPosX = oRoom.xpos;
                nPosY = oRoom.ypos;
                nPosZ = oRoom.zpos;

                // Adjust the position for the adjacent room
                switch( arExit[0] )
                {
                    case "north": nPosY -= 1; break;
                    case "south": nPosY += 1; break;
                    case "east": nPosX += 1; break;
                    case "west": nPosX -= 1; break;
                    case "northeast": nPosY -= 1; nPosX += 1; break;
                    case "northwest": nPosY -= 1; nPosX -= 1; break;
                    case "southeast": nPosY += 1; nPosX += 1; break;
                    case "southwest": nPosY += 1; nPosX -= 1; break;
                    case "up": nPosZ += 1; break;
                    case "down": nPosZ -= 1; break;
                }

                // Update the adjacent room position
                oAdj.xpos = nPosX;
                oAdj.ypos = nPosY;
                oAdj.zpos = nPosZ;

                // Add the room to the list
                pAdjacent += [ oAdj ];
            }
        }
        pRooms = pAdjacent;
    }

    // Normalise the lefthand side
    for( i = 0; i < length(pArea); i += 1 )
    {
        while( pLeft[i] > nLeft )
        {
            pArea[i] = [ null ] + pArea[i];
            pLeft[i] -= 1;
        }
    }

    // Replace the area map
    this.map = pArea;
    this.xMin = nXMin;
    this.yMin = nYMin;
    this.zMin = nZMin;
    this.xMax = nXMax;
    this.yMax = nYMax;
    this.zMax = nZMax;

    return 1;
}

[$.lib.basic.area]
RenderMap2(here) {
    // This version is used for the 'map' command.
    // It renders rooms as 3x3 cells with the room in the top-left
    // and its exits below and to the right. This means it can miss
    // some one-way exits that arrive from the north.
    result = "";
    foreach (row in this.map) {
        exits = "#o";
        x = 2; // left edge of exits line.
        foreach (room in row) {
            isTrace = (room != null);
            isClash = 0;
            if (arrayp(room)) {
                // Find the room at this Z coordinate
                found = null;
                foreach (obj in room) {
                    if (obj.zpos == here.zpos) {
                        if (found) isClash = 1;
                        found = obj;
                    }
                }
                room = found;
            } else if (room) {
                // Check if room is at this Z coordinate
                if (room.zpos != here.zpos) {
                    room = null;
                }
            }

            if (room) {
                if (room == here) { sRoom = "*"; sCol = "#Y"; }
                else { sRoom = "o"; sCol = "#g"; }
                E = S = SE = " ";
                foreach (dir in room.exits) {
                    exitCol = "#o";
                    if (dir[2].hidden) exitCol = "#K"; // exit class hidden.
                    switch (dir[0]) {
                        case "north":
                        case "northeast":
                        case "northwest":
                            // adding a north exit to the previous line would be really hard.
                            break;
                        case "south": S = exitCol+"|"; break;
                        case "east": E = exitCol+"-"; break;
                        case "west":
                            // add the E exit to the previous room.
                            if (result[-1] == " ") {
                                result = result[0..-2]+exitCol+"-";
                            }
                            break;
                        case "southeast": SE = exitCol+"\\"; break;
                        case "southwest":
                            // fix the SE exit to the left.
                            prev = exits[-1];
                            if (prev == "\\") prev = "X"; else prev = exitCol+"/";
                            exits = exits[0..-2]+prev;
                            break;
                        case "up":
                        case "down":
                            if (dir[2].hidden) sCol = "#K";
                            else sCol = "#W";
                            break;
                    }
                }
                if (isClash) sCol = "#R";
                result += sCol + sRoom + E;
                exits += S + SE;
            } else {
                // No room at this X,Y on this floor.
                if (isTrace) result += "#K. "; else result += "  ";
                exits += "  ";
            }
            x += 1;
        }
        result += "\n" + exits + "\n";
    }
    return result;
}


[$.lib.basic.area]
GenerateImmMap(me, z, reveal) {
    // based on room.GenerateMap
    // this has been optimised for the way the mud language works.
    area = this;
    map = area.map;
    // determine the rectange of rooms to render.
    // these must be odd numbers so the map has a center room.
    w = (area.xMax - area.xMin) + 1;
    h = (area.yMax - area.yMin) + 1;
    ox = 0;
    oy = 0;
    ex = w - 1;
    ey = h - 1;
    if (z == null) z = me.location.zpos;
    // render rooms and exits into a list.
    w = w * 2 - 1; // ascii map width.
    h = h * 2 - 1; // ascii map height.
    t = w*h;
    res = new array(t);
    while (length(res)<t) append(res,null); // work-around for new array limit (fixme)
    p = 0;
    for (y = oy; y <= ey; y += 1) {
        row = map[y]; // get this row.
        for (x = ox; x <= ex; x += 1) {
            // find the room on the map at these coords.
            C = room = null;
            cell = row[x]; // get this col.
            if (cell) {
                // one or more rooms at this x,y.
                if (arrayp(cell)) {
                    foreach(rm in cell) {
                        if (rm.zpos == z) {
                            room = rm; // found.
                            break;
                        } else {
                            // see the room above or below.
                            C = rm.class.trace;
                        }
                    }
                } else {
                    if (cell.zpos == z) {
                        room = cell; // found.
                    } else {
                        // see the room above or below.
                        C = cell.class.trace;
                    }
                }
            }
            if (room) {
                // set the room icon for this room.
                C = room.class.icon;
                // modify the map to add exits around this room.
                // use + or x for a closed door.
                foreach (dir in room.exits) {
                    if (dir[2].hidden) continue; // exit class hidden.
                    switch (dir[0]) {
                        case "north":
                            if (y>oy) res[p-w] = dir[2].iconNS;
                            break;
                        case "south":
                            if (y<ey) res[p+w] = dir[2].iconNS;
                            break;
                        case "east":
                            if (x<ex) res[p+1] = dir[2].iconEW;
                            break;
                        case "west":
                            if (x>ox) res[p-1] = dir[2].iconEW;
                            break;
                        case "northeast":
                            if (y>oy && x<ex) {
                                // NE cell can contain a SE or SW icon, or X.
                                // if not empty and not SW, convert to X.
                                t = p-w+1; ch = res[t];
                                if (!ch) res[t] = dir[2].iconNE; // was empty.
                                else if (search(ch,"/") < 0) res[t] = dir[2].iconX;
                            }
                            break;
                        case "northwest":
                            if (y>oy && x>ox) {
                                // NW cell can contain a SE or SW icon, or X.
                                // if not empty and not SE, convert to X.
                                t = p-w-1; ch = res[t];
                                if (!ch) res[t] = dir[2].iconSE; // NW, was empty.
                                else if (search(ch,"\\") < 0) res[t] = dir[2].iconX;
                            }
                            break;
                        case "southeast":
                            if (y<ey && x<ex) {
                                // SE cell cannot be populated yet,
                                // because we are rendering left-to-right.
                                res[p+w+1] = dir[2].iconSE;
                            }
                            break;
                        case "southwest":
                            if (y<ey && x>ox) {
                                // SW cell can contain SE icon or nothing.
                                t = p+w-1;
                                if (res[t]) res[t] = dir[2].iconX; // was SE.
                                else res[t] = dir[2].iconNE; // SW only.
                            }
                            break;
                        case "up":
                        case "down":
                            C = room.class.iconUD;
                            break;
                    }
                }
                // replace the room icon with the player icon.
                if (room == me.location) C = room.class.col+"@";
            }
            // set the room cell and advance to the next room.
            res[p] = C;
            p += 2; // next room cell.
        }
        p += w - 1; // skip one line, adjust for +=2 above.
    }
    // fill in any empty cells with spaces.
    t = length(res);
    for (p = 0; p < t; p += 1) {
        if (!res[p]) res[p] = " ";
    }
    // generate 'h' lines of ascii.
    s = "";
    t = w - 1; // generate 'w' cols of ascii (-1 for slicing)
    p = 0;
    for (y = 0; y < h; y += 1) {
        // take a slice out of the array, because join()
        // does not let us specify the start and end offsets (to fix)
        s += join(res[p..p+t],"") + "\n";
        p += w; // next ascii line.
    }
    return s + "\n";
}

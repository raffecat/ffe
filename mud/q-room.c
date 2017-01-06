[$.lib.basic.room]
-> $.lib.basic.container

area = null
capacity = null
class = $.mud.class.room.room
comments = null
desc = "No description."
emptymap = ["         ", "         ", "    #Y*    ", "         ", "         "]
exits = []
items = []
items2 = []
light = 0
minimap = null
name = "Nowhere"
noclone = 1
nodrop = 1
noget = 1
opacity = 0
short = "Nowhere"
xpos = 0
ypos = 0
zpos = 0

[$.lib.basic.room]
Describe( oIgnore )
{
    // Get the room description
    // Use desc2 to simplify base and derived room descriptions
    if( this.desc2 ) sResult = this.desc + " " + this.desc2;
    else sResult = this.desc;
    sResult += "\n";

    // Add the list of players and items
    sThings = this.ListContents( oIgnore );
    if (sThings) sResult = sResult + "\n" + sThings;

    return sResult;
}

DescribeDaylight()
{
    if( !this.class ) return "no light";
    return this.class.lightOptions[this.light];
}

[$.lib.basic.room]
DescribeWeather()
{
    return "Raining";
}

[$.lib.basic.room]
GenerateMap(me) {
    // this has been optimised for the way the mud language works.
    // record first visit to this room.
    vis = me.visited;
    if (!vis[this]) {
        if (!vis) {
            vis = new mapping;
            me.visited = vis;
        }
        vis[this] = 1;
    }
    area = this.area;
    map = area.map;
    // determine the rectange of rooms to render.
    // these must be odd numbers so the map has a center room.
    w = 5;
    h = 3;
    ox = (this.xpos - area.xMin) - to_int(w/2);
    ex = ox + w - 1;
    oy = (this.ypos - area.yMin) - to_int(h/2);
    ey = oy + h - 1;
    z = this.zpos;
    // render rooms and exits into a list.
    w = w * 2 - 1; // ascii map width.
    h = h * 2 - 1; // ascii map height.
    res = new array(w*h);
    p = 0;
    for (y = oy; y <= ey; y += 1) {
        row = null;
        if (y >= 0) row = map[y]; // get this row.
        for (x = ox; x <= ex; x += 1) {
            // find the room on the map at these coords.
            C = room = null;
            if (x >= 0) {
                cell = row[x]; // get this col.
                if (cell) {
                    // one or more rooms at this x,y.
                    if (arrayp(cell)) {
                        foreach(rm in cell) {
                            if (rm.zpos == z) {
                                room = rm; // found.
                                break;
                            } else if (vis[rm]) {
                                // see the room above or below.
                                C = rm.class.trace;
                            }
                        }
                    } else {
                        if (cell.zpos == z) {
                            room = cell; // found.
                        } else if (vis[cell]) {
                            // see the room above or below.
                            C = cell.class.trace;
                        }
                    }
                }
            }
            //me.Message("C is "+C+" at "+x+", "+y+", "+z+"\n");
            if (room && vis[room]) {
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
    map = [];
    t = w - 1; // generate 'w' cols of ascii (-1 for slicing)
    p = 0;
    for (y = 0; y < h; y += 1) {
        // take a slice out of the array, because join()
        // does not let us specify the start and end offsets (to fix)
        append(map, join(res[p..p+t],""));
        p += w; // next ascii line.
    }
    return map;
}

chat I had this idea. We can let you mark rooms into zones within the area, and then say, okay all the main rooms are zone A, and this alley is zone B, and some other little part is zone C.
chat Zone A can never see the other zoned rooms, but anyone in zone B (in the alley) can always see zone A (outside the alley) as well.
chat Now we can also do unlocks: once you discover zone B (move bins), you can see it on your map from zone A as well.


exec s=""; r=$.mud.class.room; foreach (cls in r) s += r[cls].trace +"#n\n"; this.Message(s); return 1;
exec s=""; r=$.mud.class.exit; foreach (cls in r) s += r[cls].col+cls+"#n "; return s;
exec s=""; r=$.mud.class.room; foreach (cls in r) s += r[cls].col+cls+"#n "; return s;

exec r=$.mud.class.room; foreach (id in r) { cls=r[id]; if (!cls.col) cls.col = "#w"; } return;
exec r=$.mud.class.room; foreach (id in r) { cls=r[id]; if (!cls.icon) cls.icon = cls.col+"o"; } return;
exec r=$.mud.class.room; foreach (id in r) { cls=r[id]; if (!cls.iconUD) cls.iconUD = cls.col+"O"; } return;
exec r=$.mud.class.room; foreach (id in r) { cls=r[id]; cls.trace = replace(replace(cls.icon,"o","."),"~","."); } return;

exec r=$.mud.class.exit; foreach (id in r) { cls=r[id]; if (!cls.col) cls.col = "#w"; } return;
exec r=$.mud.class.exit; foreach (id in r) { cls=r[id]; if (!cls.iconNS) cls.iconNS = cls.col+"|"; } return;
exec r=$.mud.class.exit; foreach (id in r) { cls=r[id]; if (!cls.iconEW) cls.iconEW = cls.col+"-"; } return;
exec r=$.mud.class.exit; foreach (id in r) { cls=r[id]; if (!cls.iconNE) cls.iconNE = cls.col+"/"; } return;
exec r=$.mud.class.exit; foreach (id in r) { cls=r[id]; if (!cls.iconSE) cls.iconSE = cls.col+"\\"; } return;
exec r=$.mud.class.exit; foreach (id in r) { cls=r[id]; if (!cls.iconX) cls.iconX = cls.col+"X"; } return;

exits: #yavenue#n #gclearing#n #Kdoor#n #Kdoorway#n #Gforest#n #Whall#n #whallway#n #whidden#n #Khole#n #Wladder#n #Knarrow#n #nnone#n #ypath#n #yplaza#n #Mportal#n #yroad#n #wroom#n #Ksearch#n #wshelter#n #Wstairs#n #Wsteps#n #Bstream#n
rooms: #wair#n #Galpine#n #Warctic#n #Ybeach#n #Kcave#n #wcavern#n #ocity#n #wcliff#n #Bcreek#n #Bdeep#n #Ydesert#n #Gforest#n #ggrove#n #Whall#n #Whut#n #Gice#n #Blake#n #wmountain#n #Cocean#n #opath#n #Wpeak#n #oplain#n #Briver#n #yroad#n #Krock#n #wroom#n #wscree#n #Bshallow#n #Wstairs#n #Wsteps#n #bswamp#n #otown#n #otrack#n #ctundra#n #gveldt#n #Bwater#n


[$.lib.basic.room]
GetExitPlex() {
    // Generate a 3x3 character grid for this room, showing the
    // room and all visible exits, coloured appropriately.
    // This can be cached in the room until exits change.
    C="#go";
    N=E=S=W=NE=NW=SE=SW="   ";
    foreach (dir in this.exits) {
        if (dir[2].hidden) continue; // exit class hidden.
        switch (dir[0]) {
            case "north": N="#o|"; break;
            case "south": S="#o|"; break;
            case "east": E="#o-"; break;
            case "west": W="#o-"; break;
            case "northeast": NE="#o/"; break;
            case "southeast": SE="#o\\"; break;
            case "northwest": NW="#o\\"; break;
            case "southwest": SW="#o/"; break;
            case "up": case "down": C = "#Wo"; break;
        }
    }
    return [NW,N,NE,W,C,E,SW,S,SE];
}

[$.lib.basic.room]
MigrateExits() {
    exits = this.exits;
    if (arrayp(exits)) {
        map = new mapping;
        foreach (entry in exits) {
            dir = entry[0];
            id = this.exitNameToID[dir];
            if (!id) { $.save.user.mario.Message("Bad exit: "+to_string(dir)+" "+object_name(this)); }
            info = new mapping;
            info["name"] = dir;
            info["opp"] = this.oppositeExitID[id];
            info["to"] = entry[1];
            info["type"] = entry[2];
            if (entry[3]) info["leave"] = entry[3];
            if (entry[4]) info["arrive"] = entry[4];
            if (entry[5]) info["msg"] = entry[5];
            map[id] = info;
        }
        exits = map;
    }
    return exits;
}


[$.lib.basic.room]
GetExitSummary()
{
    N=E=S=W=U=D=NE=NW=SE=SW=0;
    foreach (dir in this.exits) {
        if (dir[2].hidden) continue; // exit class hidden.
        switch (dir[0]) {
            case "north": N=1; break;
            case "south": S=1; break;
            case "east": E=1; break;
            case "west": W=1; break;
            case "northeast": NE=1; break;
            case "southeast": SE=1; break;
            case "northwest": NW=1; break;
            case "southwest": SW=1; break;
            case "up": U=1; break;
            case "down": D=1; break;
        }
    }
    card = "";
    if (W) card += "W";
    if (N) card += "N";
    if (S) card += "S";
    if (E) card += "E";
    diag = "";
    sep = "";
    if (NW) { diag += "NW"; sep = ","; }
    if (SW) { diag += sep+"SW"; sep = ","; }
    if (NE) { diag += sep+"NE"; sep = ","; }
    if (SE) { diag += sep+"SE"; }
    vert = "";
    if (U) vert += "U";
    if (D) vert += "D";
    result = "#W" + card;
    if (diag) {
        if (!card) result += "-";
        result += "#n|" + diag;
        if (vert) result += "#n|#W" + vert;
    } else if (vert) {
        if (card) result += "#n|#W";
        result += vert;
    }
    return result+"#n";
}


    result = "#W" + card;
    result += "#n|" + diag;
    if (card && (diag||vert)) result += "#n|";
    if (NW||SW||NE||SE) result += "|";
    //if ((N||S||E||W||U||D) && (NW||SW||NE||SE)) result += "|";
    if (NE) { if (Y) { result += "#KNE"; Y=0; } else { result += "#yNE"; Y=1; } }
    if (SE) { if (Y) { result += "#KSE"; Y=0; } else { result += "#ySE"; Y=1; } }
    if (NW) result += "#yNW"; else result += "-";
    if (N) result += "#WN";
    if (NE) result += "#yNE";
    if (E) result += "#WE";
    if (SE) result += "#ySE";
    if (S) result += "#WS";
    if (SW) result += "#ySW";
    if (W) result += "#WW";
    if (U) result += "#WU";
    if (D) result += "#WD";
    if( result == "" ) return "#Knone#n";


LookItem( sName )
{
    // Locate a room item by full or partial noun (id)

    sMatch = null;
    sName = lower_case(sName);

    pAdjs = split( sName, " " );
    sNoun = pAdjs[-1];
    pAdjs = pAdjs[0..-2];
    sWholeNoun = sNoun + ",";

    arItems = this.items;
    if( this.items2 ) arItems += this.items2;

    // Find the first exact match
    foreach( arInfo in arItems )
    {
        if( search( arInfo[1], sWholeNoun ) >= 0 )
        {
            // Found a plural match
            sMatch = arInfo[4];
        }
        else if( search( arInfo[0], sWholeNoun ) >= 0 )
        {
            // Found an exact match
            sMatch = arInfo[4];
        }

        if( sMatch )
        {
            // Check that all adjectives also match
            sThisAdjs = arInfo[2];
            foreach( sAdj in pAdjs )
            {
                if( search( sThisAdjs, sAdj ) < 0 )
                {
                    // At least one does not match
                    sMatch = null;
                    break;
                }
            }

            if( sMatch ) break; // success
        }
    }

    // If no match yet, find the first partial match
    if( !sMatch )
    {
        foreach( arInfo in arItems )
        {
            if( search( arInfo[0], sNoun ) >= 0 )
            {
                // Found a partial match
                sMatch = arInfo[4];
            }

            if( sMatch )
            {
                // Check that all adjectives also match
                sThisAdjs = arInfo[2];
                foreach( sAdj in pAdjs )
                {
                    if( search( sThisAdjs, sAdj ) < 0 )
                    {
                        // At least one does not match
                        sMatch = null;
                        break;
                    }
                }

                if( sMatch ) break; // success
            }
        }
    }

    return sMatch;
}


[$.lib.basic.roomclass]
-> $.lib.global

lightLevels = [0, 10, 20, 30, 40, 50, 60, 70, 80, 100]
lightOptions = ["Pitch Black", "Extremely Dark", "Very Dark", "Dark", "Dimly Lit", "Poorly Lit", "Moderately Lit", "Well Lit", "Brightly Lit", "Blindingly Lit"]
name = "room"

[$.lib.basic.container]
-> $.lib.basic.item

capacity = 0
closed = 0
container = 1
key = null
locked = 0
noclose = 1
nolock = 1
noopen = 1
nounlock = 1
opacity = 1
tolerance = 0

AddObject( oThing )
{
    // Create a 'contents' array if this object doesn't have one
    if( exists( this, "contents" ) ) this.contents += [ oThing ];
    else this.contents = [ oThing ];
}

[$.lib.basic.container]
Broadcast( msg, me, ignore )
{
    remove(this.contents, null);
    foreach(thing in this.contents) {
        if (thing != me && thing != ignore) {
            if (functionp(thing.Message)) thing.Message(msg);
        }
    }
}

CheckCapacity()
{
    // Check if contents are over capacity
    nOver = this.ContentMass() - this.capacity;
    if( nOver > 0 )
    {
        // Return chance of losing an item (0->1)
        return nOver / this.tolerance;
    }
    return 0;
}

Close()
{
    this.closed = 1;
}

ContentMass()
{
    nMass = 0;
    arContents = this.contents;
    foreach( oThing in arContents )
    {
        nMass += oThing.TotalMass();
    }
    return nMass;
}

FindMatch( sName, oIgnore, oAgent )
{
    if( this.closed && this.opacity > 0.5 )
        return "You cannot see inside " + this.TheName() + ".";

    return this.Present( sName, oIgnore, oAgent );
}

Light( nAmbient )
{
    nRadiant = this.radiant;
    nOpacity = this.opacity;

    if( nOpacity > 0.99 )
    {
        if( nRadiant < 1 ) return 0;
        nLum = (nRadiant * 10) / nAmbient;
        if( nLum > nRadiant ) return nRadiant;
        return nLum;
    }

    arContents = this.contents;
    foreach( oThing in arContents )
    {
        nRadiant += oThing.Light( nAmbient );
    }

    if( nAmbient > 0 )
    {
        nLum = (nRadiant * 10) / nAmbient;
        if( nLum <= nRadiant ) nRadiant = nLum;
    }

    return nRadiant * (1.0 - nOpacity);
}

[$.lib.basic.container]
ListContents( oAgent )
{
    sResult = "";

    if( this.closed ) sResult += this.TheName() + " is closed.\n";
    if( this.opacity > 0.5 ) return sResult;

    // Build the array of visible items
    remove( this.contents, null );
    arContents = this.contents;
    arItems = [];
    arLiving = [];
    foreach (oThing in arContents) {
        if (oThing != oAgent && oThing.visible) {
            if( oThing.living ) arLiving += [ oThing ];
            else arItems += [ oThing ];
        }
    }

    if (arLiving) {
        foreach (oThing in arLiving) {
            sResult += capitalise(oThing.Short()) + " is standing here.\n";
        }
    }

    if (arItems) {
        sList = this.ListMultipleShort(arItems, oAgent);
        if (sList) {
            if (arLiving) sResult += "\n";
            sResult += capitalise(sList)+"\n";
        }
    }

    return sResult;
}

Lock( oKey, oAgent )
{
    oKey.UseKey( this.key, 1, this, oAgent ); // 1 = lock
    this.locked = 1;
}

Open()
{
    this.closed = 0;
}

Present( sName, oIgnore, oAgent )
{
    // Locate an object by full or partial noun (id)

    oMatch = null;
    sName = lower_case(sName);
    pAdjs = split( sName, " " );
    sNoun = pAdjs[-1];
    pAdjs = pAdjs[0..-2];
    sWholeNoun = sNoun + ",";

    // Find the first exact match
    arContents = this.contents;
    foreach( oThing in arContents )
    {
        if( oThing == oIgnore ) continue;

        if( search( oThing.idplural, sWholeNoun ) >= 0 )
        {
            // Found a plural match
            oMatch = oThing;
        }
        else if( search( oThing.id, sWholeNoun ) >= 0 )
        {
            // Found an exact match
            oMatch = oThing;
        }

        if( oMatch )
        {
            // Check that all adjectives also match
            sThisAdjs = oThing.idadj;
            foreach( sAdj in pAdjs )
            {
                if( search( sThisAdjs, sAdj ) < 0 )
                {
                    // At least one does not match
                    oMatch = null;
                    break;
                }
            }

            if( oMatch ) break; // success
        }
    }

    // If no match yet, find the first partial match
    if( !oMatch )
    {
        foreach( oThing in arContents )
        {
            if( oThing == oIgnore ) continue;

            if( search( oThing.id, sNoun ) >= 0 )
            {
                // Found a partial match
                oMatch = oThing;
            }

            if( oMatch )
            {
                // Check that all adjectives also match
                sThisAdjs = oThing.idadj;
                foreach( sAdj in pAdjs )
                {
                    if( search( sThisAdjs, sAdj ) < 0 )
                    {
                        // At least one does not match
                        oMatch = null;
                        break;
                    }
                }

                if( oMatch ) break; // success
            }
        }
    }

    return oMatch;
}

RemoveObject( oThing )
{
    remove( this.contents, oThing );
}

TestClose( oAgent )
{
    if( this.noclose )
    {
        if( functionp(this.noclose) ) return this.noclose();
        if( stringp(this.noclose) ) return this.noclose;
        return 0;
    }

    if( this.closed ) return this.TheName() + " is already closed.";

    return 1;
}

TestInsert( oThing, oAgent )
{
    if( this.closed ) return this.TheName() + " is closed.";

    if( numberp(this.capacity) )
    {
        // Check if capacity will be exceeded
        if( this.ContentMass() + oThing.TotalMass() > this.capacity + this.tolerance )
            return oThing.TheName() + " will not fit in " + this.TheName() + ".";
    }

    return 1;
}

TestLock( oKey, oAgent )
{
    if( this.nolock )
    {
        if( functionp(this.nolock) ) return this.nolock();
        if( stringp(this.nolock) ) return this.nolock;
        return 0;
    }

    if( !this.key ) return "There is no lock on " + this.TheName() + ".";

    mResult = oKey.TestKey( this.key, 1, this, oAgent ); // 1 = lock
    if( mResult != 1 ) return mResult;

    if( this.locked ) return this.TheName() + " is already locked.";

    return 1;
}

TestOpen( oAgent )
{
    if( this.noopen )
    {
        if( functionp(this.noopen) ) return this.noopen();
        if( stringp(this.noopen) ) return this.noopen;
        return 0;
    }

    if( !this.closed ) return this.TheName() + " is already open.";

    return 1;
}

TestPresent( sName, oIgnore )
{
    // Locate an object by exact id
    arContents = this.contents;
    foreach( oThing in arContents )
    {
        if( oThing == oIgnore ) continue;
        if( oThing.name == sName ) return oThing;
    }
    return null;
}

TestRemove( oThing, oAgent )
{
    if( this.closed ) return this.TheName() + " is closed.";

    return 1;
}

TestUnlock( oKey, oAgent )
{
    if( this.nounlock )
    {
        if( functionp(this.nounlock) ) return this.nounlock();
        if( stringp(this.nounlock) ) return this.nounlock;
        return 0;
    }

    if( !this.key ) return "There is no lock on " + this.TheName() + ".";

    mResult = oKey.TestKey( this.key, 0, this, oAgent ); // 0 = unlock
    if( mResult != 1 ) return mResult;

    if( !this.locked ) return this.TheName() + " is already unlocked.";

    return 1;
}

TotalMass()
{
    return this.mass + this.ContentMass();
}

Unlock( oKey, oAgent )
{
    oKey.UseKey( this.key, 0, this, oAgent ); // 0 = unlock
    this.locked = 0;
}

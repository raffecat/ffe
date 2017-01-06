[$.lib.basic.item]
-> $.lib.basic.object

Browse = null
Close = null
Open = null
PostIn = null
Read = null
noclone = 0
nodrop = null
noget = null
nohold = null

Drop( oAgent )
{
    if( this.nodrop )
    {
        if( functionp(this.nodrop) ) return this.nodrop( oAgent );
        if( stringp(this.nodrop) ) return this.nodrop;
        return 0;
    }

    return 1;
}


Get( oAgent )
{
    if( this.noget )
    {
        if( functionp(this.noget) ) return this.noget( oAgent );
        if( stringp(this.noget) ) return this.noget;
        return 0;
    }

    return 1;
}


Hold( oAgent )
{
    if( this.nohold )
    {
        if( functionp(this.nohold) ) return this.nohold( oAgent );
        if( stringp(this.nohold) ) return this.nohold;
        return 0;
    }

    this.held = 1;

    return 1;
}

Put( oContain, oAgent )
{
    if( this.nodrop )
    {
        if( functionp(this.nodrop) ) return this.nodrop( oContain, oAgent );
        if( stringp(this.nodrop) ) return this.nodrop;
        return 0;
    }

    return 1;
}

UnHold( oAgent )
{
    if( this.nohold )
    {
        if( functionp(this.nohold) ) return this.nohold( oAgent );
        if( stringp(this.nohold) ) return this.nohold;
        return 0;
    }

    this.held = 0;

    return 1;
}

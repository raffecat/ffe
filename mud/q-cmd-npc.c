
[$.lib.cmd.mp.write]
-> $.lib.basic.command

Command(args, who)
{
    oCaller.Message( sArgs );
}



[$.lib.cmd.mp.mpact]
-> $.lib.basic.command

Command( args, who )
{
    if (who.location) who.location.Broadcast("#o"+args, who);
}

[$.lib.cmd.mp.mpmove]
-> $.lib.basic.command

Command( args, who )
{
    words = split(trim(args));

    // Hmm, these args need to be instances (refs to objects)

    oOldLoc = this.location;
    if (oOldLoc && functionp(oOldLoc.RemoveObject)) oOldLoc.RemoveObject(this);
    this.location = oLocation;
    if (oLocation) oLocation.AddObject( this );

    return 1;
}

[$.lib.cmd.mp.mpinsert]
-> $.lib.basic.command

eval_args(args, who)
{
    if (stringp(args)) {
        words = split(trim(args));
    }

}

Command( args, who )
{
    // 
    words = this.eval_args(args, who);

    oOldLoc = this.location;
    if (oOldLoc && functionp(oOldLoc.RemoveObject)) oOldLoc.RemoveObject(this);
    this.location = oLocation;
    if (oLocation) oLocation.AddObject( this );

    return 1;
}

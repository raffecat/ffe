[$.lib.basic.living]
-> $.lib.basic.container

class = $.mud.class.player.warrior
combatAction = null
combatLeader = null
combatMembers = []
combatStarted = 0
combatTarget = null
con = 1
creator = 0
dex = 1
en = 50
followers = null
gender = "neuter"
groupLeader = null
groupMembers = []
healthColour = ["#r", "#r", "#r", "#R", "#R", "#o", "#o", "#Y", "#Y", "#G", "#G"]
healthDescription = ["DEAD!", "Dying", "Almost Dead", "Leaking Guts", "Bleeding Freely", "Badly Wounded", "Wounded"
, "Lightly Wounded", "Injured", "Scratched", "Perfect"]
homeTown = ""
hp = 50
int = 1
living = 1
maxen = 50
maxhp = 50
maxmv = 10
mv = 10
nodrop = 1
noget = 1
race = $.mud.race.player.human
skills = {}
spd = 1
str = 1
tolerance = 0.1
wiz = 1

AddFollower( oThing )
{
    // Create a 'follower' array if this object doesn't have one
    if( exists( this, "followers" ) ) this.followers += [ oThing ];
    else this.followers = [ oThing ];
}

Assist( oTarget )
{
    if( !oTarget.combatStarted ) return 0;

    // Assist the combatant
    this.combatTarget = null;
    this.combatLeader = oTarget;
    this.combatGroup = [];
    this.combatStarted = 0;

    return 1;
}

Attack( oTarget, bCounterAttack )
{
    // Form a new combat group
    this.combatTarget = oTarget;
    this.combatLeader = null;
    this.combatGroup = [];
    this.combatStarted = 0;

    if( !bCounterAttack )
    {
        this.Message( "You leap to attack " + oTarget.TheName() + "!" );
        this.Broadcast( "$^agent$ leaps to attack $1$!", oTarget );

        // Register this combat group
        this.combatDaemon.BeginCombat( this );
    }

    return 1;
}

[$.lib.basic.living]
AttemptFollow( oTarget, sDir, oFrom, oTo )
{
    if (this.location != oFrom) {
        this.Message("#Y"+capitalise(oTarget.TheName())+" leaves you behind!");
        return 0; // false
    }
    res = this.FollowExit(sDir);
    if (res) {
        if (stringp(res)) this.Message(res);
        this.Message("#Y"+capitalise(oTarget.TheName())+" leaves you behind!");
        return 0; // false
    } else {
        this.Message("#WYou follow " + oTarget.TheName() + " " + sDir + ".");
        return 1; // true
    }
}

Broadcast( sMessage, mContext, oIgnore )
{
    // Send message to this room
    if( this.location ) this.location.Broadcast( sMessage, this, mContext, oIgnore );
}

CanAttack()
{
    if( this.hp <= 0 ) return 0;
    if( !this.location ) return 0;
    return 1;
}

CanBeAttacked( oAttacker )
{
    if( this.hp <= 0 ) return capitalise(this.TheName()) + " is already dead.";
    if( this.location != oAttacker.location ) return capitalise(this.TheName()) + " is not within reach.";
    return 1;
}

CombatRound()
{
    // Check if the attacker is still alive
    if( this.hp <= 0 ) return this.StopCombat();

    // Check if an action is pending
    if( this.combatAction )
    {
        mNext = this.combatAction( this );
        if( !mNext || functionp(mNext) ) this.combatAction = mNext;
        return;
    }

    // Check if there is a target to attack
    oTarget = this.combatTarget;
    if( oTarget )
    {
        // Check if the target is still alive
        if( oTarget.hp <= 0 ) return this.StopCombat();

        // Check if combat has already started
        if( !this.combatStarted )
        {
            // First round - attacker only
            arCombatants = [ this ];
            this.combatStarted = 1;
        }
        else
        {
            // Check if the target is free to counter-attack
            if( !oTarget.combatStarted )
            {
                oTarget.Attack( this, 1 );
            }

            // Look for allies free to auto-assist
            if( this.groupLeader ) arGroup = this.groupLeader.groupMembers;
            else arGroup = this.groupMembers;
            foreach( oAlly in arGroup )
            {
                if( oAlly == this ) continue;
                if( !oAlly.combatTarget && oAlly.option["auto-attack"] )
                {
                    oAlly.Assist( this );
                }
            }

            // Look for enemies free to auto-assist
            if( oTarget.groupLeader ) arGroup = oTarget.groupLeader.groupMembers;
            else arGroup = oTarget.groupMembers;
            foreach( oEnemy in arGroup )
            {
                if( oEnemy == oTarget ) continue;
                if( !oEnemy.combatTarget && oEnemy.option["auto-attack"] )
                {
                    oEnemy.Assist( oTarget );
                }
            }

            // Build a list of all combatants
            arCombatants = [ this, oTarget ] + this.combatMembers + oTarget.combatMembers;
        }

        // Sort the list of combatants based on speed and dexterity
        // ...

        // Perform each combatant's turn
        foreach( oCombatant in arCombatants )
        {
            // Display start of combat messages for new combatants
            if( !oCombatant.combatStarted )
            {
                oCombatant.combatStarted = 1;

                if( oCombatant == oTarget )
                {
                    oCombatant.Message( "You counterattack " + this.TheName() + "!" );
                    oCombatant.Broadcast( "$^agent$ counterattacks $1$!", this );
                }
                else if( oCombatant != this )
                {
                    oLeader = oCombatant.combatLeader;
                    oCombatant.Message( "You leap to assist " + oLeader.TheName() + "!" );
                    oCombatant.Broadcast( "$^agent$ leaps to assist $1$!", oLeader );
                    oCombatant.combatStarted = 1;
                }
            }

            // Perform the combat turn
            oCombatant.CombatTurn();

            // Check if the target was killed
            if( oTarget.hp <= 0 )
            {
                // Check if this is the group leader
                if( this.groupLeader == null )
                {
                    // Choose another target from the enemy group
                    arGroup = oTarget.groupMembers;
                    oTarget = null;
                    foreach( oEnemy in arGroup )
                    {
                        if( oEnemy.hp > 0 )
                        {
                            // Turn the group to attack this enemy
                            this.combatTarget = oTarget = oEnemy;
                            break;
                        }
                    }
                    if( !oTarget ) return this.StopCombat();
                }
                else
                {
                    // Revert to a free group member (auto-assist)
                    return this.StopCombat();
                }
            }
        }
    }
    else
    {
        // No enemy to attack
        this.StopCombat();
    }
}

CombatTurn()
{
    // Find a weapon
    if( length(this.contents) ) oWeapon = this.contents[0];
    else oWeapon = this; // bare hands

    // Find an enemy to attack
    oEnemy = this.combatTarget;
    if( !oEnemy && this.combatLeader ) oEnemy = this.combatLeader.combatTarget;

    if( oEnemy )
    {
        // Hit the enemy with a weapon
        nDamage = oEnemy.HitDamage( this, oWeapon );
        if( nDamage > 1 ) sDamage = "wounds (" + nDamage + ")";
        else sDamage = "tickles (" + nDamage + ")";

        // Determine enemy status
        sStatus = oEnemy.HealthShort();

        // Display combat messages
        this.Message( "Your sword thrust #B" + sDamage + "#n $1$ (" + oEnemy.HealthShort(1) + "#n)", this, oEnemy );
        this.Broadcast( "$^agent$'s sword thrust #o" + sDamage + "#n $1$ (" + sStatus + "#n)", oEnemy, oEnemy );
        oEnemy.Message( "$^agent$'s sword thrust #R" + sDamage + "#n you (" + sStatus + "#n)", this );

        // Check if the enemy is dead
        if( oEnemy.hp <= 0 )
        {
            this.Message( "#RYou killed " + oEnemy.TheName() + "!" );
            this.combatTarget = null;

            oEnemy.Broadcast( "#R$^agent$ is DEAD!" );
        }
    }
}

Die( oAgent, oWeapon )
{
    // Stop all combat
    this.StopCombat();

    this.Message( "#RYou are dead!" );
}

[$.lib.basic.living]
FollowExit(dir, forced)
{
    // Check for an exit direction
    here = this.location;
    if (!here) return "You are in limbo.";

    exitList = here.exits;
    foreach (entry in exitList) {
        if (entry[0] == dir) {
            // Build the messages for others to see.
            to = entry[1];
            if (!to) return 1; // missing destination.
            class = entry[2];
            if (class.hidden) {
                if (!forced) return 1; // no such exit.
            }
            leave = entry[3];
            if (!leave) {
                leave = class.out[dir];
                if (!leave) leave = class.leave;
                if (!leave) leave = "$n leaves "+dir+".";
            }
            // Find the opposite exit in the target room.
            odir = this.oppositeDir[dir];
            foreach (oppEntry in to.exits) {
                if (oppEntry[0] == odir) {
                    arrive = oppEntry[4];
                    break;
                }
            }
            opp = this.oppositeExit[dir];
            if (!arrive) {
                arrive = class.in[odir];
                if (!arrive) arrive = class.arrive;
                if (!arrive) arrive = "$n arrives from "+opp+".";
            }
            // Render both messages.
            vars = new mapping;
            vars["me"] = this;
            vars["dir"] = dir;                     // 'south'     / 'down'
            vars["to"] = this.definiteExit[dir];   // 'the south' / 'below'
            leave = this.ExpandMessage(leave, vars);
            vars["to"] = "";
            vars["dir"] = odir;                    // 'north'     / 'up'
            vars["from"] = this.oppositeExit[dir]; // 'the north' / 'above'
            arrive = this.ExpandMessage(arrive, vars);

            msg = entry[5];
            if (!msg) msg = class.msg;

            // Move to the new location
            res = this.Move(to, leave, arrive, this, msg);
            if (res == 1) {
                followers = [];
                foreach(u in this.followers) { if (u) append(followers, u); } // copy list.
                foreach(shadow in followers) {
                    if (!shadow.AttemptFollow(this, dir, here, to)) {
                        remove(this.followers, shadow);
                    }
                }
            }
            return 0; // success.
        }
    }
    return 1; // no such exit.
}

GroupLeader()
{
    if( this.groupLeader ) return this.groupLeader;
    return this;
}

HealthShort( bUseColour )
{
    hp = this.hp;
    if( hp <= 0 ) hp = 0;
    else if( hp > this.maxhp ) hp = 10;
    else hp = 1 + to_int( ((hp-1) * 10) / this.maxhp );

    if( bUseColour ) return this.healthColour[hp] + this.healthDescription[hp] + "#n";
    return this.healthDescription[hp];
}

HitDamage( oAgent, oWeapon )
{
    if( this.hp <= 0 ) return 0;

    // Calculate the damage
    nSkill = oAgent.skills["sword"];
    nBase = oWeapon.damage;
    nDelta = (nBase / 2) * (nSkill / 5) - 5; // +/- 25% based on skill
    nVariance = ( random(nBase) / 5 ) - (nBase / 10); // random +/- 10% of base
    nDamage = nBase + nDelta + nVariance;

    // Subtract the damage from health
    if( nDamage < 0 ) nDamage = 0;
    this.hp -= nDamage;
    if( this.hp <= 0 ) this.Die( oAgent, oWeapon );

    return nDamage;
}

IsFollowing( oThing, oAgent )
{
    //return (this.followers && search( this.followers, oThing ) != -1);
    if( this.followers ) foreach( o in this.followers ) if( o == oThing ) return 1;
    return 0;
}

Message(m)
{
}

[$.lib.basic.living]
MoveRandom()
{
    // Pick an exit at random and follow it.
    len = length(this.location.exits);
    if (len) {
        dir = this.location.exits[random(len)];
        return this.FollowExit(dir[0]);
    }
}

MoveRandom()
{
    // Pick an exit at random and follow it
    if( !this.location ) return;
    exitList = this.location.exits; nLen = sizeof(exitList); if (nLen) entry = exitList[ random(nLen) ];
    {
        entry = exitList[ random(nLen) ];
        return this.FollowExit( entry[0] );
    }
}

Name()
{
    return this.name;
}

RawMessage()
{
}

RemoveFollower( oThing )
{
    remove(this.followers, oThing);
}

Short( oAgent )
{
    sResult = this.AName( oAgent, 1 );

    if( this.gender || this.race || this.class )
    {
        sResult += ", a";
        if( this.gender ) sResult += " " + this.gender;
        if( this.race ) sResult += " " + this.race.name;
        if( this.class ) sResult += " " + this.class.name;
    }

    return sResult;
}

StopCombat()
{
    // Disband this combat group
    this.combatTarget = null;
    this.combatLeader = null;
    this.combatGroup = [];
    this.combatStarted = 0;

    // Unregister this combat group
    this.combatDaemon.EndCombat( this );

    return 1;
}

TestFollow( oThing, oAgent )
{
    return 1;
}


[$.lib.basic.race]
-> $.lib.global

homeTown = "Nowhere"
name = "creature"


[$.lib.basic.class]
-> $.lib.global

name = "generic"

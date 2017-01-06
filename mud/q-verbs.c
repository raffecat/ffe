[$.verbs.fail]
-> $.globals
func (args, me, vars) {
  // fail <message>
  return args || 1;
}

[$.verbs.if-exists]
-> $.globals
func (args, vars) {
  // if-exists <path> <message>
  words = split(args," ",0,1);
  if (this.find_path(words[0], vars)) {
    // fail with the message.
    return join(words[1..]," ");
  }
  return 0;
}

[$.verbs.must-exist]
-> $.globals
func (args, vars) {
  // must-exist <path> <message>
  words = split(args," ",0,1);
  if (!this.find_path(words[0], vars)) {
    // fail with the message.
    return join(words[1..]," ");
  }
  return 0;
}

[$.verbs.echo]
-> $.globals
func (args, vars) {
  // echo <target> <message>
  words = split(args," ",1,1); // split one word
  target = words[0];
  msg = words[1];
  if (target[0]=="$") target = vars[target[1..]]; // handle $var
  if (target == "others") {
    target = "room";
    ignore = vars["self"];
  }
  who = vars[target];
  // render for each player.
  msg = this.ExpandMessage(msg, vars);
  if (who.isRoom) {
    foreach(obj in who.contents) {
      if (obj.isPlayer) {
        if (obj == ignore) continue;
        fn = obj.Message;
        if (fn) {
          fn(msg);
        }
      }
    }
  } else {
    fn = who.Message;
    if (fn) {
      fn(msg);
    }
  }
  return 0;
}

[$.verbs.render]
-> $.globals
func (args, vars) {
  // render <target> <template>
  words = split(args," ",1,1);
  target = words[0];
  name = words[1];
  if (target[0]=="$") target = vars[target[1..]]; // handle $var
  if (target == "others") {
    target = "room";
    ignore = vars["self"];
  }
  who = vars[target];
  // find the template.
  template = vars["@verb"].templateCache[name];
  if (!template) return "No template: "+name;
  // render for each player.
  if (who.isRoom) {
    foreach(obj in who.contents) {
      if (obj.isPlayer) {
        if (obj == ignore) continue;
        fn = obj.MessageNoWrap;
        if (fn) {
          msg = this.RenderTemplate(template, vars, obj);
          fn(msg);
        }
      }
    }
  } else {
    fn = who.MessageNoWrap;
    if (fn) {
      msg = this.RenderTemplate(template, vars, who);
      fn(msg);
    }
  }
  return 0;
}

[$.verbs.set-tmp]
-> $.globals
func (args, vars) {
  if (!vars["self"].admin) return "No.";
  // set-tmp <path> <name> <template source>
  words = split(args," ",2,1);
  obj = this.find_path(args[0]);
  name = args[1];
  source = args[2];
  if (!obj) return "Object not found: "+args[0];
  if (!name) return "Must specify template name: "+name;
  if (source) {
    // compile the template.
    func = this.CompileTemplate(args);
    if (!functionp(func)) return "Template is not valid.";
    // save the template.
    if (!exists(obj,"templates")) obj.templates = new mapping;
    if (!exists(obj,"templateCache")) obj.templates = new mapping;
    obj.templates[name] = source;
    obj.templateCache[name] = func;
  } else {
    remove(obj.templates, name);
    remove(obj.templateCache, name);
  }
  return 0;
}

[$.verbs.wait]
-> $.globals
func (args, vars) {
  num = this.RollDice(trim(args));
  if (num && num >= 1) sleep(num);
  return 0;
}

[$.verbs.go]
-> $.globals
func (args, vars) {
  dir = this.validExits[args];
  if (!dir) return "You cannot go that way.";
  me = vars["self"];
  if (!functionp(me.FollowExit)) return "You cannot move.";
  res = me.FollowExit(dir);
  if (res) {
    if (stringp(res)) return res;
    return "You cannot go " + dir + " from here.";
  }
  return 0;
}

[$.verbs.u]
cmds = "go up"

[$.verbs.d]
cmds = "go down"

[$.verbs.n]
cmds = "go north"

[$.verbs.s]
cmds = "go south"

[$.verbs.e]
cmds = "go east"

[$.verbs.w]
cmds = "go west"

[$.verbs.ne]
cmds = "go northeast"

[$.verbs.nw]
cmds = "go northwest"

[$.verbs.se]
cmds = "go southeast"

[$.verbs.sw]
cmds = "go southwest"

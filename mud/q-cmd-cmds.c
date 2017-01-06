pastet $.verbs.cmds.list-verbs 1
#m |
#m-+----------------
#m |#C Quintiqua Verbs:  {if pat}#n(filter: {pat}){end if}
#m-+------------------------------------
#m |#n Use 'verb add <name>' to add a verb.
#m |
{each verb in verbs}{cols 16}#m | #Y{verb.id}{end cols}{end each}


pastet $.lib.cmd.imm.cmds.show-verb 1
#YYou are editing:#W {verb.id}#n

{if verb.alias}Alias:       {verb.alias}
{end if}{if verb.func}Func:        {verb.func}
{end if}{if verb.need}Need:        {verb.need}
{end if}{if verb.opt}Option:      {verb.opt}
{end if}{if verb.extra}Extra:       {verb.extra}
{end if}Access:      {verb.access}
Perms:       {verb.perms}
Commands:
{if verb.command}  [native code]
{end if}{each line in verb.cmds}  {line}
{end each}
Templates:
{each name in verb.templates}  {name}
{end each}


[$.lib.cmd.imm.cmds]
-> $.lib.basic.command



verbList = $.verbs;

usage = "Unknown sub-command. Enter 'help cmds' for help."

help = "Edit commands.\n"
       "\n"
       "cmds [search]                 List all commands.\n"
       "cmds <cmd>                    Show command properties.\n"
       "cmds <cmd> set <list>         Set the command list for a command.\n"
       "cmds <cmd> def <name> <list>  Set extra command lists for a command.\n"
       "cmds <cmd> tmp <name>         Set (paste in) a template for 'render'.\n"
       "\n"

[$.lib.cmd.imm.cmds]
Command(args, me) {
  args = split(args," ",0,1);
  name = args[0];
  sub = args[1];
  if (sub) {
    func = this["cmd_"+sub];
    if (stringp(func)) func = this["cmd_"+func]; // alias.
    if (!functionp(func)) return this.usage;
    return func(name, this.verbList[name], args, me);
  }
  // list commands.
  items = this.verbList;
  res = [];
  foreach(key in items) {
    if (search(key, name) >= 0) append(res, items[key]);
  }
  if (!res) return "No matching verbs: "+name;
  if (length(res) > 1) {
    // list commands.
    vars = new mapping;
    vars["verbs"] = res;
    vars["pat"] = name;
    me.MessageNoWrap(this.RenderTemplate(this.templateCache["list-verbs"], vars, me));
  } else {
    // show one command.
    vars = new mapping;
    vars["verb"] = res[0];
    me.MessageNoWrap(this.RenderTemplate(this.templateCache["show-verb"], vars, me));
  }
  return 1;
}

[$.lib.cmd.imm.cmds]
cmd_rename(name, cmd, args, me) {
  if (!me.admin) return "What?";
  to = args[2];
  if (!cmd) return return "Command does not exist: "+name;
  if (!to) return "Usage: cmds <cmd> rename <name>   Enter 'help cmds' for help.";
  if (exists(this.verbList, to)) return "Command already exists: "+to;
  move_object(cmd, this.verbList, to);
  me.Message("Changed command from '"+name+"' to '"+to+"'.");
  return 1;
}

[$.lib.cmd.imm.cmds]
cmd_delete(name, cmd, args, me) {
  if (!me.admin) return "What?";
  if (!cmd) return return "Command does not exist: "+name;
  verbs = this.verbList;
  delete verbs[name];
  remove(verbs, name);
  me.Message("Deleted command: "+name);
  return 1;
}

[$.lib.cmd.imm.cmds]
cmd_add(name, cmd, args, me) {
  if (!me.admin) return "What?";
  if (exists(this.verbList, to)) return "Command already exists: "+to;

  id = args[1];
  req = to_int(args[2]);
  if (!id) return "Usage: verb add <name> <access>   Enter 'verb help' for help.";
  if (this.verbs[id]) return "Verb already exists: "+id;
  if (skip(id, 0, "abcdefghijklmnopqrstuvwxyz-") >= 0) {
    return "Verbs must use only lower-case letters.";
  }
  if (!req || req < 1 || to_string(req) != args[2]) {
    return "Must provide an access level (1=player, 2=mob, 10=imm)";
  }
  verb = new object(id, this.verbs);
  verb.access = req;
  verb.perms = req;
  verb.func = "cmd";
  verb.cmds = [];
  me.editVerb = verb;
  return this.cmd_show(args, me);
}

[$.lib.cmd.imm.cmds]
cmd_show(args, me) {
  verb = me.editVerb;
  if (!verb) return "You are not editing a verb. Enter 'verb help' for help.";
  vars = new mapping;
  vars["verb"] = verb;
  me.MessageNoWrap(this.RenderTemplate(this.templateCache["show-verb"], vars, me));
  return 1;
}

[$.lib.cmd.imm.cmds]
cmd_cmds(args, me) {
  verb = me.editVerb;
  if (!verb) return "You are not editing a verb. Enter 'verb help' for help.";
  me.Message("\n#YYou are editing verb:#W "+object_name(verb)+"#n\n\n");
  lines = split(join(args[1..], " "), ";", 0, 1);
  res = [];
  foreach (line in lines) { s = trim(line); if (s) append(res, s); }
  if (!res) return "No commands entered.";
  verb.cmds = res;
  me.Message("Saved command list.");
  return 1;
}

[$.lib.cmd.imm.cmds]
cmd_func(args, me) {
  verb = me.editVerb;
  if (!verb) return "You are not editing a verb. Enter 'verb help' for help.";
  me.Message("\n#YYou are editing verb:#W "+object_name(verb)+"#n\n\n");
  func = args[1];
  if (!func) return "Must specify a func name.";
  verb.func = func;
  me.Message("Set verb func to: "+func);
  return 1;
}

[$.lib.cmd.imm.cmds]
cmd_tmp(args, me) {
  verb = me.editVerb;
  if (!verb) return "You are not editing a verb. Enter 'verb help' for help.";
  name = args[1];
  if (!name) return "Usage: verb tmp <name> [show|delete]";
  if (args[2] && args[2] != "-d") {
    if (args[2] == "del" || args[2] == "delete") {
      if (!verb.templates[name]) return "Template does not exist: "+name;
      remove(verb.templates, name);
      remove(verb.templateCache, name);
      me.Message("Removed template: "+name);
      return 1;
    }
    if (args[2] == "show") {
      tmp = verb.templates[name];
      if (!tmp) return "Template does not exist: "+name;
      me.RawMessage(join(tmp,"\n"));
      return 1;
    }
    return "Usage: verb tmp <name> [show|delete]";
  }
  me.Message("\n#YYou are editing verb:#W "+object_name(verb)+"#n\n");
  me.Message("Paste in your template now. Enter '.' to finish.");
  // read lines.
  lines = [];
  repeat {
    me.RawMessage(">");
    line = read_line(me.loginSocket);
    if (line == ".") break;
    lines += [ line ];
  }
  if (!lines) return "No lines entered.";
  // compile template.
  src = join(lines, "\n");
  warn = null; if (args[2]) warn = me.RawMessage;
  func = this.CompileTemplate(src, warn);
  if (stringp(func)) return "Template did not compile.";
  if (!verb.templates) verb.templates = new mapping;
  if (!verb.templateCache) verb.templateCache = new mapping;
  verb.templates[name] = lines;
  verb.templateCache[name] = func;
  me.Message("Saved template: "+name);
  return 1;
}

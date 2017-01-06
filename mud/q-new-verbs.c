@faerryn.radakara.rainforest7
@faerryn.clothes.linen-shirt
@human.calydynia.oubliette
@human.armour.chest.chainmail

[$.verbs.drop]
variants = [
  ["drop", "$item", [
    "find-my $item",
    "send-to $item drop | You cannot drop $item."]]
]

[$.verbs.give]
variants = [
  ["give", "$item to $target", [
    "find-my $item",
    "find-living $target",
    "send-to $target give-to $item | You cannot give $item to $target."]]
]
pattern = [" to ",[1,"give","item","target"]]

[$.verbs.put]
variants = [
  ["in", "$item in $target", [
    "find-my $item",
    "find-any $target",
    "send-to $target put-in $item | You cannot put $item in $target."]],
  ["on", "$item on $target", [
    "find-my $item",
    "find-any $target",
    "send-to $target put-on $item | You cannot put $item on $target."]]
]
pattern = [" in ",[1,"put-in","item","target"]," on ",[1,"put-on","item","target"]]

--------------------------------------------------------

The verb {

verb put

The verb 'put' has the following variants:

  in: $item in $target
    1. find-my $item
    2. find-any $target
    3. send-to $target put-in $item | You cannot put $item in $target.

  on: $item on $target
    1. find-my $item
    2. find-any $target
    3. send-to $target put-on $item | You cannot put $item on $target.

verb get

The verb 'get' is an alias for 'take'.

verb take

The verb 'take' has the following variants:

  from: $item from $target
    1.


[$.verbs.get]
alias = "take"

[$.verbs.take]
pattern = [1,"item", {
  "from": [1,"target", [
    "find-any $target",
    "find-in $target $item",
    "send-to $item take-from $target | You cannot take $item from $target."
  ]],
  "": [[
    "find-any $item",
    "send-to $item take | You cannot take $item."
  ]]
}]

[$.verbs.browse]
alias = "do-direct"
vars = {
  "event": "browse",
  "msg": "browse through"
}

[$.verbs.open]
pattern = [1,"item", [
  "find-any $item",
  "send-to $item open | You cannot open $item."
]]

[$.verbs.close]
pattern = [1,"item", [
  "find-any $item",
  "send-to $item close | You cannot close $item."
]]

[$.verbs.lock]
pattern = [1,"item", [
  "find-any $item",
  "send-to $item lock | You cannot lock $item."
]]

[$.verbs.unlock]
pattern = [1,"item", [
  "find-any $item",
  "send-to $item unlock | You cannot unlock $item."
]]

[$.verbs.get]
alias = "wield"

[$.verbs.wield]
pattern = [1,"item", [
  "find-any $item",
  "send-to $item wield | You cannot wield $item."
]]

[$.verbs.wear]
pattern = [1,"item", [
  "find-any $item",
  "send-to $item wear | You cannot wear $item."
]]


[$.verbs.bgs]
pattern = [{
  "add": [[1,"id"], [
    "check-id $id",
    "ensure-new @spell.backgrounds.$id Background already exists: $id",
    "echo Paste in your ascii art now. Enter '.' to finish.",
    "paste-raw @spell.backgrounds.$id",
    "bgs $id"
  ]],
  "update": [[1,"id"], [
    "ensure-exists @spell.backgrounds.$id Background does not exist: $id",
    "echo Paste in your ascii art now. Enter '.' to finish.",
    "paste-raw @spell.backgrounds.$id",
    "bgs $id"
  ]]
  "*": [[], [
    ""
  ]]
}]

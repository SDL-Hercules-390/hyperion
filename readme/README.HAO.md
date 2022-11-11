![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# Hercules Automatic Operator

## Contents

1. [About](#About)
2. [Defining a rule](#Defining-a-rule)
3. [Substituting substrings in the command](#Substituting-substrings-in-the-command)
4. [Other commands and limitations](#Other-commands-and-limitations)
5. [Giving credit where credit is due](#Giving-credit-where-credit-is-due)

## About

The Hercules Automatic Operator (HAO) feature is a facility that allows one to automatically issue panel commands in response to certain messages being issued.

To use the Hercules Automatic Operator facility, one first defines a "rule" consisting of a "target" and an associated "command". The "target" is just a regular expression pattern used to match against the text of the various messages that Hercules issues as it runs. Whenever a match is found, the rule "fires" and its associated command is automatically issued.

The Hercules Automatic Operator facility is **only** for those messages issued **by Hercules** to its HMC (hardware console). It **cannot** be used for whatever messages the guest operating system may issue to any of its terminals. It is only a **Hercules** automatic operator and **not** a "VSE", "MVS", "VM", etc, automatic operator!

## Defining a rule

To define a HAO rule, first enter the command:

 
    hao  <tgt>
    
    
to define the rule's "target" match pattern (a simple regular expression), and then immediately follow it with the command:


    hao  <cmd>


to define the rule's associated panel-command. 

The target pattern is a simple regular expression value as defined by whatever regular expression your host platform build happens to support. For Windows it must be a Perl Compatible Regular Expression (PCRE).
For other supported build platforms it might be some other supported regular expression syntax. Check your host platform's programming documentation for further details.

The associated panel-command <cmd> is whatever valid Hercules command you wish to execute in response to a message being issued that matches the specified pattern (target). (Okay, that's not completely true. The "command" is actually any text string one desires. It doesn't have to be a valid Hercules command, but it makes infinitely more sense if it is one).

## Substituting substrings in the command
Substrings may be substituted into a rule's associated panel-command <cmd> by means of the special variables $1, $2, etc, which will be replaced by the values of "capturing groups" in the match pattern. A capturing group is a part of the regular expression enclosed in parentheses which is matched with text in the target message. In this way, commands may be constructed which contain substrings extracted from the message which triggered the command.

The following special variables are recognized:

    $1 to $9      the text which matched the 1st to 9th capturing group in the target regular expression.
    $`            the text preceding the regular expression match.
    $'            the text following the regular expression match.
    $$            replaced by a single dollar sign.

Note that substitution of a $n variable does not occur if there are fewer than n capturing groups in the regular expression.

As an example, the rule below issues the Hercules command `i 001F` in response to the Hercules message "**`HHC01090I 0:001F COMM: client 127.0.0.1 devtype 3270: connection reset`**":

    hao tgt HHC01090I .:([0-9A-F]{4}) COMM: .* connection reset
    hao cmd i $1

Another example, shown below, illustrates how the dot matrix display of a 3590(?) tape unit might be used to implement an automatic tape library in response to the Hercules message "**`HHC00224I 0:0581 Tape file *, type HET: display "K2DSBK2 " / "M2DSBK3S" (alternating)`**":

    hao tgt HHC00224I .:([0-9A-F]{4}) Tape file .*: display (?:".{8}" \/ )?"M([A-Z0-9]{1,6})\s*S"
    hao cmd devinit $1 /u/tapes/$2.aws

Which would result in the Hercules command "`devinit 0581 /u/tapes/2DSBK3.aws`" being automatically issued.

More information about Perl Compatible Regular Expression (PCRE) syntax (as well as a nice online web page that allows you to test your expressions) can be found here:

  *  [PCRE Regex Cheatsheet](https://www.debuggex.com/cheatsheet/regex/pcre)
  *  [Online PCRE test page](https://regex101.com/)

## Other commands and limitations

ALL defined rules are checked for a match each time Hercules issues a message. There is no way to specify "stop processing subsequent rules". If a message is issued that matches two or more rules, each associated command is then issued in sequence.

The current implementation limits the total number of defined rules to 64. If you need to define more than 64 rules you will either have to build Hercules for yourself (increasing the value of the `HAO_MAXRULE` constant in [hao.c](../hao.c)) or else beg one of the Hercules developers to please do it for you.

To delete a fully or partially defined HAO rule, first use the `hao list` command to list all of the defined (or partially defined) rules, and then use the `hao del <nnn>` command to delete the specific rule identified by 'nnn'. (All rules are assigned numbers as they are defined and are thus identified by their numeric value). Optionally, one may delete ALL defined or partially defined rules by issuing the command `hao clear`.

## Giving credit where credit is due

The Hercules Automatic Operator facility was designed eons ago (or so it seems anyway) by Bernard van der Helm of Noordwijkerhout, The Netherlands, and is thus copyrighted by him. All I did was FINALLY get around to implementing it in the current release of Hercules. If you like it and find it to be useful, I think it'd be real nice if you'd let Bernard know that. Thanks.

-- Fish
February 2021

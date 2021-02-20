# text-editor
A simple text editor inspired by [ed](https://en.wikipedia.org/wiki/Ed_(text_editor)).

The editor interface consists of text commands, terminated by a newline. Commands can be followed by a portion of text, consisting of one or more lines, terminated by a character . (period) that appears as the only character on the next line. The commands consist of single letter, in some cases preceded by one or two integers. The editor interface is freely inspired by that of the historic unix editor ed.

## Commands

### Change
`(ind1, ind2) c`
Change the text present on the lines between `ind1` and `ind2` (extremes included). The text following the command must be a number of lines equal to `ind2 - ind1 + 1`. `ind1` must actually be either an address present in the text, or the first address after the last line presentin the text (or 1 if the text is still empty).
 
### Delete
`(ind1, ind2) d`

Delete the lines between `ind1` and `ind2` (extremes included), by moving upwards the lines following that of address `ind2` (if any). Deleting a line that does not exist in the text has no effect.

### Print
`(ind1, ind2) p`

Print the lines between `ind1` and `ind2` (or between `ind1` and the end of the textif `ind2` does not match any address in the text). For each row not existing in the region between `ind1` and `ind2` (inclusive) the editor will print a line containing only the dot character.

### Undo
`(number) u`

Performs the undo of a number of commands (c or d) equal to the one specified in parentheses (where number is an integer greater than zero). A sequence of consecutive undo commands cancels a number of steps equal to the sum of the steps specified in each one of them. If the number of commands to cancel ishigher than that of the commands executed, all steps are canceled.The execution of a text editing command (c, d) after an undocancels the effects of the permanently canceled commands. Note that in the number of commands to cancel the commands that are also counted have no effect (for example deleting a block of lines that do not exist).

### Redo
`(number) r`

Undo the effect of undo for an equal number of commands starting from the current version (redo function). Note what number it must bean integer strictly greater than zero. We therefore have that a sequence of commands like `10u 5r`. It is to all effects equivalent to the `5u` command only. Similarly, the sequence `12u 3r 2u 7r` equivalent to the `4u` command. In the event that the number of commands if the redo is higher than the currently canceled ones, the editor will do the maximum number of redos possible. 

### Quit
`q`

Stop the editor.

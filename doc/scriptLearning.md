
```
REF_STR code/renderergl2/glsl/bokeh_fp.glsl
REF_CC build/release-linux-x86_64/renderergl2/glsl/bokeh_fp.c
REF_STR code/renderergl2/glsl/bokeh_vp.glsl
REF_CC build/release-linux-x86_64/renderergl2/glsl/bokeh_vp.c
```

```
define DO_REF_STR
$(echo_cmd) "REF_STR $<"
$(Q)rm -f $@
$(Q)echo "const char *fallbackShader_$(notdir $(basename $<)) =" >> $@
$(Q)cat $< | sed -e 's/^/\"/;s/$$/\\n\"/' | tr -d '\r' >> $@
$(Q)echo ";" >> $@
endef
```


## what the principle of DO\_REF\_STR ?


* $@ : 目标文件
* $^ : 所有的依赖文件
* $< : 第一个依赖文件
* basename : Print NAME with any leading directory components removed. If specified, also remove a trailing SUFFIX.

* $(notdir names...) : 
Extracts all but the directory-part of each file name in names. 
If the file name contains no slash, it is left unchanged. 
Otherwise, everything through the last slash is removed from it.
A file name that ends with a slash becomes an empty string. 
This is unfortunate, because it means that the result does not 
always have the same number of whitespace-separated file names 
as the argument had; but we do not see any other valid alternative.

* sed, a stream editor :
A stream editor is used to perform basic text transformations on
an input stream (a file or input from a pipeline). While in some
ways similar to an editor which permits scripted edits (such as ed),
sed works by making only one pass over the input(s), and is consequently
more efficient. But it is sed’s ability to filter text in a pipeline
which particularly distinguishes it from other types of editors.

```
-e script
--expression=script
# Add the commands in script to the set of commands 
# to be run while processing the input
```

### sed script overview

A sed program consists of one or more sed commands, passed in by
one or more of the -e, -f, --expression, and --file options, 
or the first non-option argument if zero of these options are used

sed commands follow this syntax:

```
[addr]X[options]

# X is a single-letter sed command. 
# [addr] is an optional line address. If [addr] is specified, 
# the command X will be executed only on the matched lines. 
# [addr] can be a single line number, a regular expression, 
# or a range of lines (see sed addresses). 
# Additional [options] are used for some sed commands.

# Commands within a script or script-file can be separated
# by semicolons (;) or newlines (ASCII 10). Multiple scripts
# can be specified with -e or -f options.
```


### Addresses overview

Addresses determine on which line(s) the sed command will be executed.
The following command replaces the word ‘hello’ with ‘world’ only on line 13:

```
sed '13s/hello/world/' input.txt > output.txt
```

If no addresses are given, the command is performed on all lines.
The following command replaces the word ‘hello’ with ‘world’ on 
all lines in the input file.



The following example prints all input until a line starting with
the word ‘foo’ is found. If such line is found, sed will terminate
with exit status 42. If such line was not found (and no other error
occurred), sed will exit with status 0. /^foo/ is a regular-expression
address. q is the quit command. 42 is the command option.

```
sed '/^foo/q42' input.txt > output.txt
```

The following examples are all equivalent. They perform two sed
operations: deleting any lines matching the regular expression
/^foo/, and replacing all occurrences of the string ‘hello’ with ‘world’:

```
sed '/^foo/d ; s/hello/world/' input.txt > output.txt

sed -e '/^foo/d' -e 's/hello/world/' input.txt > output.txt

echo '/^foo/d' > script.sed
echo 's/hello/world/' >> script.sed
sed -f script.sed input.txt > output.txt

echo 's/hello/world/' > script2.sed
sed -e '/^foo/d' -f script2.sed input.txt > output.txt
```

```
sed -e 's/^/\"/;s/$$/\\n\"/'

#equivalent to

sed -e 's/^/\"/' -e 's/$$/\\n\"/'
```

###  The "s" Command

The s command (as in substitute) is probably the most important in sed
and has a lot of different options. The syntax of the s command is
```
s/regexp/replacement/[flags]
```

(substitute) Match the regular-expression against the content of 
the pattern space. If found, replace matched string with replacement.

Its basic concept is simple: the s command attempts to match the pattern space
against the supplied regular expression regexp; if the match is successful,
then that portion of the pattern space which was matched is replaced with replacement.

定位符使您能够将正则表达式固定到行首或行尾。它们还使您能够创建这样的正则表达式
，这些正则表达式出现在一个单词内、在一个单词的开头或者一个单词的结尾。
定位符用来描述字符串或单词的边界，^ 和 $ 分别指字符串的开始与结束

```
s/^/\"/ 表示每一行的开始插入 " 字符串。
```

```
's/$$/\\n\"/' 表示每一行的结束插入 \n" 字符串。
```

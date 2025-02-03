x5:-b.
x5:-not c.
d:-c.
c:-d.
x5|d:-not not b.
a:-x5.
c:-a.
b:-c.
%% { "options": ["0"]
%% , "solutions": [["a","b","c","d","x5"]] }

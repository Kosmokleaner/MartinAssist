see tweet: https://twitter.com/MittringMartin/status/1635354734981963777

MartinAssist 3/11/2023

caller can implement this:

struct ICppParserSink
{
  virtual void onInclude(const char* path, bool local) = 0;
  virtual void onCommentLine(const char* line) = 0;



done for recreational programming

literate programming syntax:
* <label> 0..9 a..z A..Z _ /
* <component> "any C++ string even multi line but for now now only a..z A..Z 0..9 _ / space"

* // #begin <label>
  to start define a part
* // #end <label>
  to end define a part, label is optional, without lable it will close last begin
* // #use/here/anchor/label <label>
  to define anchor where parts can be added
* // #ignore
  to ignore the line 
* // ##### <component>
  the following characters define a component/patch, it can be applied or not building a tree of variants



EveryHere app:
* Store a list of all files with creation date/size/path, possibly hash to find redundancy level, browse and to ensure you have at least n backups
* database can be in one spot e.g. dropbox

todo EFUs
* show drive scanned in x days, show date in tooltip instead
* where are other dups ?
* checkbox for recycle bin filter
* add more files to fill the view to always show vertical scrollbar
* icon window has some bug, seems to repeat after half the data
* filter box should have lens icon and disc borders
* compute and show redundancy
* multi select drives should add all files to the files window
* editable drive description



todo:
* Delete multiselect devices
* compute line number for parse errors
* iterate include hierarchy
* support unicode
* parse hlsl files
* unit tests

done:
* include hierarchy
* parse // comments and return with interface
* parse /**/ comments and return with interface
* parse all files in a folder hierarchy



C++ / HLSL / file parser / HTML ?

helping me on unreaveling large code base, marking what is known and finding relevant spots again, find todo
64Bit 

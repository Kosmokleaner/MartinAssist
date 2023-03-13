see tweet: https://twitter.com/MittringMartin/status/1635354734981963777

MartinAssist 3/11/2023

caller can implement this:

struct ICppParserSink
{
  virtual void onInclude(const char* path, bool local) = 0;
  virtual void onCommentLine(const char* line) = 0;



done for recreational programming

todo:
* compute line number for parse errors
* iterate include hierarchy
* support unicode
* parse hlsl files
* unit tests

done
* include hierarchy
* parse // comments and return with interface
* parse /**/ comments and return with interface
* parse all files in a folder hierarchy



C++ / HLSL / file parser / HTML ?

helping me on unreaveling large code base, marking what is known and finding relevant spots again
64Bit 

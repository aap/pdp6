/* just a subset here */
enum ItemType
{
	Nothing = 0,
	Code    = 1,
	Symbols = 2,
	Entry   = 4,
	End     = 5,
	Name    = 6,
	Start   = 7,
};

enum SymType
{
	SymName    = 000,
	SymGlobal  = 004,
	SymLocal   = 010,
	SymBlock   = 014,
	SymGlobalH = 044,	/* hidden
	SymLocalH  = 050,	 */
	SymUndef   = 060,
};

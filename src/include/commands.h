#pragma once

enum Command
{
	Connect = 0,
	Disconnect,
	Cancel,
	List,
	RecList, // Recursive list
	Transfer,
	Delete,
	RemoveDir,
	MakeDir,
	Rename,
	Chmod,
	Raw
};

struct t_commandFlags
{
	bool chainable;
};

#ifdef ENGINE
t_commandFlags commandFlags[12] = {
	false,
	false,
	false,
	false,
	false,
	false,
	true,
	false,
	true,
	true,
	true,
	false
};
#endif
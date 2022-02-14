#include "stdafx.h"
#pragma hdrstop

#include "SoundRender_Core.h"
#include "SoundRender_Source.h"

CSoundRender_Source*	CSoundRender_Core::i_create_source		(LPCSTR name)
{
	//// Search
	//string256			id;
	//strlwr				(strcpy(id,name));
	//if (strext(id))		*strext(id) = 0;
	//for (u32 it=0; it<s_sources.size(); it++)		{
	//	if (0==xr_strcmp(*s_sources[it]->fname,id))	return s_sources[it];
	//}

	//// Load a _new one
	//CSoundRender_Source* S	= xr_new<CSoundRender_Source>	();
	//S->load					(id);
	//s_sources.push_back		(S);
	//return S;

	// Search
	string256 id;
	strcpy(id, name);
	xr_strlwr(id);
	if (strext(id))
		*strext(id) = 0;
	auto it = s_sources.find(id);
	if (it != s_sources.end())
	{
		return it->second;
	}

	// Load a _new one
	CSoundRender_Source* S = new CSoundRender_Source();
	S->load(id);
	s_sources.insert({ id, S });
	return S;
}

void					CSoundRender_Core::i_destroy_source		(CSoundRender_Source*  S)
{
	// No actual destroy at all
}

void CSoundRender_Core::i_create_all_sources()
{
#ifndef MASTER_GOLD
	CTimer T;
	T.Start();
#endif

	FS_FileSet flist;
	FS.file_list(flist, "$game_sounds$", FS_ListFiles, "*.ogg");
	const size_t sizeBefore = s_sources.size();

	const auto processFile = [&](const FS_File& file)
	{
		string256 id;
		strcpy(id, file.name.c_str());

		xr_strlwr(id);
		if (strext(id))
			*strext(id) = 0;

		{
			const auto it = s_sources.find(id);
			if (it != s_sources.end())
				return;
		}

		CSoundRender_Source* S = new CSoundRender_Source();
		S->load(id);

		s_sources.insert({ id, S });
	};

#ifndef MASTER_GOLD
	Msg("Finished creating %d sound sources. Duration: %d ms",
		s_sources.size() - sizeBefore, T.GetElapsed_ms());
#endif
}
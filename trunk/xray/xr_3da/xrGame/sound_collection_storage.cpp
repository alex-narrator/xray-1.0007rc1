////////////////////////////////////////////////////////////////////////////
//	Module 		: sound_collection_storage.cpp
//	Created 	: 13.10.2005
//  Modified 	: 13.10.2005
//	Author		: Dmitriy Iassenev
//	Description : sound collection storage
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sound_collection_storage.h"
#include "object_broker.h"

CSoundCollectionStorage	*g_sound_collection_storage = 0;

std::size_t hash_value(const CSoundCollectionStorage::CSoundCollectionParams& k)
{
	using std::size_t;
	using std::hash;

	// Compute individual hash values for first,
	// second and third and combine them using XOR
	// and bit shifting:
	return (((hash<size_t>()(k.m_sound_prefix._get()->dwCRC)
		^ (hash<size_t>()(k.m_sound_player_prefix._get()->dwCRC) << 1)) >> 1)
		^ (hash<size_t>()((size_t)k.m_type) << 1) >> 1)
		^ (hash<size_t>()((size_t)k.m_max_count) << 1);
}

bool operator<(const CSoundCollectionStorage::CSoundCollectionParams& lhs, const CSoundCollectionStorage::CSoundCollectionParams& rhs)
{
	if (lhs.m_sound_prefix != rhs.m_sound_prefix)
		return lhs.m_sound_prefix < rhs.m_sound_prefix;

	if (lhs.m_sound_player_prefix != rhs.m_sound_player_prefix)
		return lhs.m_sound_player_prefix < rhs.m_sound_player_prefix;

	if (lhs.m_max_count != rhs.m_max_count)
		return lhs.m_max_count < rhs.m_max_count;

	return lhs.m_type < rhs.m_type;
}

CSoundCollectionStorage::~CSoundCollectionStorage	()
{
	delete_data				(m_objects);
}

CSoundPlayer::CSoundCollection* CSoundCollectionStorage::object(const CSoundCollectionParams &params)
{
	OBJECTS::const_iterator I = m_objects.find(params);
	if (I != m_objects.end())
		return I->second;

	CSoundCollection* collection = xr_new<CSoundCollection>(params);
	m_objects.insert(std::make_pair(params, collection));
	return collection;
}

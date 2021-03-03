#ifndef FS_POSE_H
#define FS_POSE_H

#include "llsingleton.h"

class FSPose : public LLSingleton<FSPose>
{
	LOG_CLASS(FSPose);

	LLSINGLETON(FSPose);
	~FSPose();

public:
	void setPose(const std::string& new_pose, bool save_state = true);
	void stopPose();

private:
	LLUUID	mCurrentPose;
};

#endif // FS_POSE_H

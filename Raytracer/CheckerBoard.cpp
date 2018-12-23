#include "CheckerBoard.h"

re::real re::CheckerBoard::SampleNormalized(const Vector3 & point)
{
	bool thresold[3] =
	{
		point.X <= 0.5,
		0,
		point.Z <= 0.5,
	};



	return thresold[2] ^ (thresold[0] ^ thresold[1]);

}

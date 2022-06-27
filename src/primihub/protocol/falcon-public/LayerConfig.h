
#pragma once
#include "globals.h"

namespace primihub{
    namespace falcon
{
class LayerConfig
{
public:
	std::string type;
	LayerConfig(std::string _type):type(_type) {};
};
}// namespace primihub{
}

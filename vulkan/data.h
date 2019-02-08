#ifndef _DATA_H_
#define _DATA_h_

#include <vector>

struct render_components
{
    struct render_component_data
    {
	float *matrix;
    };

    std::vector<render_component_data> components;

    void
    update(void);
};

struct all_components
{
    render_components renders;
    //    ...
};

#endif



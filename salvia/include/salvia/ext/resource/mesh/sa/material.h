#ifndef SALVIAX_MATERIAL_H
#define SALVIAX_MATERIAL_H

#include <salviax/include/resource/resource_forward.h>

#include <eflib/platform/typedefs.h>
#include <salviax/include/resource/mesh/sa/mesh.h>
#include <string>

namespace salviax::resource {

class obj_material : public attached_data {
public:
  obj_material();

  std::string name;

  eflib::vec4 ambient;
  eflib::vec4 diffuse;
  eflib::vec4 specular;

  int shininess;
  float alpha;

  bool is_specular;

  std::string tex_name;
  salviar::texture_ptr tex;
  ~obj_material() {}
};

} // namespace salviax::resource

#endif

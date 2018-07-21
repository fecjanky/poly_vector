[![Build Status](https://travis-ci.org/fecjanky/poly_vector.svg?branch=master)](https://travis-ci.org/fecjanky/poly_vector)  [![Coverage Status](https://coveralls.io/repos/github/fecjanky/poly_vector/badge.svg?branch=master)](https://coveralls.io/github/fecjanky/poly_vector?branch=master)
# poly_vector

poly_vector is an std::vector like class template tailored for storing polymorphic objects derived 
from a well-known interface of which the class is parametrized in.

## Usage 

The class template is header only, so it could be used multiple ways:
 * download the raw file, add to your project and/or include it directly
 * build and install the cmake package then use the library in your CMakeLists.txt file:
 
```cmake
find_package(PolyVector CONFIG REQUIRED)

add_executable(MyExecutable  ${MyExecutable_SRC})

target_link_libraries(MyExecutable PolyVector)

 ```


## Example

Given a class hierarchy as illustrated by the figure below:

![Sample Hierarchy][hierarchy]

[hierarchy]: doc/images/demo_hierarchy.png "Class hierarchy example"

One can store implementations of Interface interface in a ```poly_vector<Interface>``` container

```cpp
...
using namespace poly;

poly_vector<Interface> v;

v.push_back(ImplA());
v.emplace_back<ImplB>();
v.emplace_back<ImplC>();

for(const Interface& i : v){
// Invoke doSomething on all objects
    i.doSomething();
}

// remove the last element
v.pop_back();

// invoke doSomething() on ImplB object
v.back().doSomething();

...

```


## Benefits

```poly_vector``` manages the underlying storage as a single chunk of memory, therefore it provides the following benefits:
* reduced allocation count compared to ```std::vector<std::unique_ptr<Interface>>>``` based alternative
* increased sequential access performance due to locality of references enforced by the structure automatically without the need of custom allocators
* less typing when pushing objects into the the container


The storage layout differences are illustrated by the figures below:


![](doc/images/std_vec.png)
*```std::vector<std::unique_ptr<Interface>>``` memory layout*


![](doc/images/poly_vec_fig.png)
*```poly::poly_vector<Interface>``` memory layout*


## Debug Visualiztion support

So far the container has only Visual Studio debugger visualization support. 
If you are using the cmake package and building with Visual Studio the natvis file will be added 
to your project automatically. Otherwise you'll have to install the natvis file manually 
from visualizer/poly_vector.natvis

![](doc/images/natvis.png)
*Natvis debug visualization*

## TODOs
* GDB pretty printer 


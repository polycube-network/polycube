/*
Copyright (c) 2014 Ashley Jeffs

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

// Package gabs implements a simplified wrapper around creating and parsing JSON.
package gabs2
//aa
import (
	"encoding/json"
	"errors"
	"github.com/iancoleman/orderedmap"
)

//--------------------------------------------------------------------------------------------------

var (
	// ErrNotObjOrArray - The target is not an object or array type.
	ErrNotObjOrArray = errors.New("not an object or array")

	// ErrNotObj - The target is not an object type.
	ErrNotObj = errors.New("not an object")
)

//--------------------------------------------------------------------------------------------------

// Container - an internal structure that holds a reference to the core interface map of the parsed
// json. Use this container to move context.
type Container struct {
	object interface{}
}

// Data - Return the contained data as an interface{}.
func (g *Container) Data() interface{} {
	if g == nil {
		return nil
	}
	return g.object
}

//--------------------------------------------------------------------------------------------------

// Search - Attempt to find and return an object within the JSON structure by specifying the
// hierarchy of field names to locate the target. If the search encounters an array and has not
// reached the end target then it will iterate each object of the array for the target and return
// all of the results in a JSON array.
func (g *Container) Search(hierarchy ...string) *Container {
	var object interface{}

	object = g.Data()
	for target := 0; target < len(hierarchy); target++ {
		if mmap, ok := object.(orderedmap.OrderedMap); ok {
			object, ok = mmap.Get(hierarchy[target])
			if !ok {
				return nil
			}
		} else if marray, ok := object.([]interface{}); ok {
			tmpArray := []interface{}{}
			for _, val := range marray {
				tmpGabs := &Container{val}
				res := tmpGabs.Search(hierarchy[target:]...)
				if res != nil {
					tmpArray = append(tmpArray, res.Data())
				}
			}
			if len(tmpArray) == 0 {
				return nil
			}
			return &Container{tmpArray}
		} else {
			return nil
		}
	}


	return &Container{object}
}

// S - Shorthand method, does the same thing as Search.
func (g *Container) S(hierarchy ...string) *Container {
	return g.Search(hierarchy...)
}

// Children - Return a slice of all the children of the array. This also works for objects, however,
// the children returned for an object will NOT be in order and you lose the names of the returned
// objects this way.
func (g *Container) Children() ([]*Container, error) {
	if array, ok := g.Data().([]interface{}); ok {
		children := make([]*Container, len(array))
		for i := 0; i < len(array); i++ {
			children[i] = &Container{array[i]}
		}
		return children, nil
	}
	if array, ok := g.Data().([]orderedmap.OrderedMap); ok {
		children := make([]*Container, len(array))
		for i := 0; i < len(array); i++ {
			children[i] = &Container{array[i]}
		}
		return children, nil
	}
	if mmap, ok := g.Data().(orderedmap.OrderedMap); ok {
		children := []*Container{}
		for _, k := range mmap.Keys() {
			obj, _ := mmap.Get(k)
			children = append(children, &Container{obj})
		}
		return children, nil
	}
	return nil, ErrNotObjOrArray
}

// ChildrenMap - Return a map of all the children of an object.
func (g *Container) ChildrenMap() (*orderedmap.OrderedMap, error) {
	if mmap, ok := g.Data().(orderedmap.OrderedMap); ok {
		children := *orderedmap.New()
		for _, name := range mmap.Keys() {
			obj, _ := mmap.Get(name)
			children.Set(name, &Container{obj})
		}
		return &children, nil
	}
	return nil, ErrNotObj
}

//--------------------------------------------------------------------------------------------------

// Bytes - Converts the contained object back to a JSON []byte blob.
func (g *Container) Bytes() []byte {
	if g.Data() != nil {
		if bytes, err := json.Marshal(g.object); err == nil {
			return bytes
		}
	}
	return []byte("{}")
}

// BytesIndent - Converts the contained object to a JSON []byte blob formatted with prefix, indent.
func (g *Container) BytesIndent(prefix string, indent string) []byte {
	if g.object != nil {
		if bytes, err := json.MarshalIndent(g.object, prefix, indent); err == nil {
			return bytes
		}
	}
	return []byte("{}")
}

// String - Converts the contained object to a JSON formatted string.
func (g *Container) String() string {
	return string(g.Bytes())
}

// StringIndent - Converts the contained object back to a JSON formatted string with prefix, indent.
func (g *Container) StringIndent(prefix string, indent string) string {
	return string(g.BytesIndent(prefix, indent))
}


// New - Create a new gabs JSON object.
func New() *Container {
	o := orderedmap.New();
	return &Container{*o}
}

// ParseJSON - Convert a string into a representation of the parsed JSON.
func ParseJSON(sample []byte) (*Container, error) {
	// try to decode as json object
	var err error
	o := orderedmap.New();
	err = json.Unmarshal(sample, o)
	if err == nil {
		return &Container{*o}, nil
	}

	// try to decode as an array
	// this is a workaround to parse internal maps as orderedMaps instead of
	// standard golang maps
	var array []interface{}
	err = json.Unmarshal(sample, &array)
	if err == nil {
		var ret []interface{}
		for _, x := range array {
			json, _ := json.Marshal(x)
			c, _ := ParseJSON(json)
			ret = append(ret, c.object)
		}
		return &Container{ret}, nil
	}

	// let use the default decoder
	var gabs Container

	err = json.Unmarshal(sample, &gabs.object)
	if err == nil {
		return &gabs, nil
	}

	return nil, err
}
//--------------------------------------------------------------------------------------------------

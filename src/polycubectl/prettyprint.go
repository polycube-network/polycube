/*
 * Copyright 2018 The Polycube Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package main

import (
	"fmt"
	"reflect"
	"strconv"

	"github.com/Jeffail/gabs2"
	"github.com/iancoleman/orderedmap"
	"github.com/ryanuber/columnize"
)

var (
	buffer string
)

// Takes an array of bytes representing a json object and converts
// it do a format easy to understand by a human.
// The output is ordered, first leafs, then objects and finally arrays.
// The deep parameter defines the level of detail of the child objects that is printed.
func PrettyPrint(j []byte, hideList map[string]bool, deep int) ([]byte, error) {
	jsonParsed, err := gabs2.ParseJSON(j)
	if err != nil {
		return nil, err
	}

	print(0, "", hideList, jsonParsed, deep)
	return []byte(buffer), nil
}

// Takes a map of arrays of strings and converts it to a table.
// Each entry in the map is a column and the key is the header of the colum.
func toTable(indent int, path string, hideList map[string]bool,
	o *orderedmap.OrderedMap) string {
	config := columnize.DefaultConfig()
	config.NoTrim = true
	config.Prefix = fmt.Sprintf("%*s", indent, "")
	//config.Glue = "|"

	numRows := 0
	numColumns := 0

	// get size of the output
	key := o.Keys()
	numColumns = len(key)
	for _, k := range key {
		if _, ok := hideList[path+k]; ok {
			continue
		}
		v, _ := o.Get(k)
		if len(v.([]string)) > numRows {
			numRows = len(v.([]string))
		}
	}

	// make room for header
	numRows += 1

	// allocate memory for final table
	format := make([][]string, numRows)
	for i := 0; i < numRows; i++ {
		format[i] = make([]string, numColumns)
	}

	for i, k := range key {
		if _, ok := hideList[path+k]; ok {
			continue
		}

		// header
		format[0][i] = k

		v, _ := o.Get(k)
		val := v.([]string)
		for j, v0 := range val {
			format[j+1][i] = v0
		}
	}

	return columnize.Format(format, config)
}

func getObjectValues(slice *gabs2.Container, deep int) *orderedmap.OrderedMap {
	o := orderedmap.New()

	if deep == 0 {
		return o
	}

	children, _ := slice.ChildrenMap()

	// collect leafs' values
	for _, key := range children.Keys() {
		child_, _ := children.Get(key)
		child := child_.(*gabs2.Container)
		if reflect.ValueOf(child.Data()).Kind() != reflect.Struct &&
			reflect.ValueOf(child.Data()).Kind() != reflect.Slice {
			_, ok := o.Get(key)
			if !ok {
				o.Set(key, []string{})
			}

			v, _ := o.Get(key)
			val := append(v.([]string), formatValue(child.Data()))
			o.Set(key, val)
		}
	}

	// collect slices values
	for _, key := range children.Keys() {
		child_, _ := children.Get(key)
		child := child_.(*gabs2.Container)
		if reflect.ValueOf(child.Data()).Kind() == reflect.Slice {
			if deep > 1 {
				s := getSliceValues(child, deep)
				keys := s.Keys()
				for _, k_ := range keys {
					vS, _ := s.Get(k_)
					k := key
					if k_ != "" {
						k = fmt.Sprintf("%s (%s)", k_, key)
					}
					// check if the key is already present, if not add it
					v, ok := o.Get(k)
					if !ok {
						o.Set(k, []string{})
					}

					v, _ = o.Get(k)
					val := append(v.([]string), vS.([]string)...)
					o.Set(k, val)
				}
			} else {
				children0, _ := child.Children()
				o.Set(key, []string{fmt.Sprintf("[%d items]", len(children0))})
			}
		}
	}

	return o
}

func getSliceValues(slice *gabs2.Container, deep int) *orderedmap.OrderedMap {
	o := orderedmap.New()

	size := 0 // total number of rows
	children, _ := slice.Children()
	for _, child := range children {
		current_size := size
		size_delta := 0
		if reflect.ValueOf(child.Data()).Kind() == reflect.Struct {
			s := getObjectValues(child, deep-1)
			// merge into full map
			keys := s.Keys()
			for _, k := range keys {
				vS, _ := s.Get(k)
				if len(vS.([]string)) > size_delta {
					size_delta = len(vS.([]string))
				}

				v, ok := o.Get(k)
				if !ok {
					o.Set(k, []string{})
				}

				v, _ = o.Get(k)
				val := v.([]string)
				// add stub elements to fill spaces
				if len(val) < current_size {
					val = append(val, make([]string, current_size-len(val))...)
				}
				val = append(val, vS.([]string)...)
				o.Set(k, val)
			}
		} else if reflect.ValueOf(child.Data()).Kind() == reflect.String {
			k := "" // special value to indicate not key
			v, ok := o.Get(k)
			if !ok {
				o.Set(k, []string{})
			}
			v, _ = o.Get(k)
			val := v.([]string)
			// add stub elements to fill spaces
			if len(val) < current_size {
				val = append(val, make([]string, current_size-len(val))...)
			}
			val = append(val, formatValue(child.Data()))
			o.Set(k, val)
			size_delta = 1
		}

		size += size_delta
	}

	return o
}

func printSlice(indent int, path string, hideList map[string]bool,
	json *gabs2.Container, deep int) string {
	var buf string

	if deep <= 0 {
		return buf
	}
	values := getSliceValues(json, deep)

	buf += toTable(indent, path, hideList, values) + "\n"
	return buf
}

func print(indent int, path string, hideList map[string]bool,
	json *gabs2.Container, deep int) {

	if deep <= 0 {
		return
	}

	// is the element a leaf?
	if reflect.ValueOf(json.Data()).Kind() != reflect.Slice &&
		reflect.ValueOf(json.Data()).Kind() != reflect.Array &&
		reflect.ValueOf(json.Data()).Kind() != reflect.Struct &&
		reflect.ValueOf(json.Data()).Kind() != reflect.Map {
		buffer += fmt.Sprintf("%*s%v\n", indent, "", formatValue(json.Data()))
	} else if reflect.ValueOf(json.Data()).Kind() == reflect.Slice ||
		reflect.ValueOf(json.Data()).Kind() == reflect.Array {
		buffer += printSlice(indent, path, hideList, json, deep)
	} else if reflect.ValueOf(json.Data()).Kind() == reflect.Struct {
		// This element is a struct, print
		//  leafs
		// 	objects
		// 	arrays

		// print leafs
		children, _ := json.ChildrenMap()
		for _, key := range children.Keys() {
			child_, _ := children.Get(key)
			child := child_.(*gabs2.Container)
			if reflect.ValueOf(child.Data()).Kind() != reflect.Struct &&
				reflect.ValueOf(child.Data()).Kind() != reflect.Slice {
				if _, ok := hideList[path+key]; ok {
					continue
				}
				buffer += fmt.Sprintf("%*s%v: %v\n", indent, "", key, formatValue(child.Data()))
			}
		}

		// print objects
		for _, key := range children.Keys() {
			child_, _ := children.Get(key)
			child := child_.(*gabs2.Container)
			if reflect.ValueOf(child.Data()).Kind() == reflect.Struct {
				buffer += fmt.Sprintf("%*s%v:\n", indent, "", key)
				print(indent+1, path+key+".", hideList, child, deep)
			}
		}

		// print arrays
		for _, key := range children.Keys() {
		    //Links are not printed because they are not intended for humans
		    if key == "_links"{
		        continue;
		    }
			child_, _ := children.Get(key)
			child := child_.(*gabs2.Container)
			if reflect.ValueOf(child.Data()).Kind() == reflect.Slice {
				if _, ok := hideList[path+key]; ok {
					continue
				}
				if deep > 1 {
					buffer += fmt.Sprintf("\n%*s%v:\n", indent, "", key)
					buffer += printSlice(indent+1, path+key+".", hideList, child, deep)
				} else {
					children0, _ := child.Children()
					buffer += fmt.Sprintf("%*s%v:", indent, "", key)
					buffer += fmt.Sprintf(" [%d items]\n", len(children0))
				}
			}
		}
	}
}

// if data is floating point, print dot notation human readable
func formatValue(data interface{}) string {
	buf := ""
	buf = fmt.Sprintf("%v", data)

    if reflect.ValueOf(data).Kind() == reflect.String {
        return buf
    }

	_, erri := strconv.ParseInt(buf, 10, 64)
	if erri != nil {
		f, errf := strconv.ParseFloat(buf, 64)
		if errf == nil {
			buf = fmt.Sprintf("%f", f)
		}
	}

	return buf
}

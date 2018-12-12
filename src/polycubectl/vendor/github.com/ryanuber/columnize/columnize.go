package columnize

import (
	"bytes"
	"fmt"
	"strings"
	"github.com/buger/goterm"
)

// Config can be used to tune certain parameters which affect the way
// in which Columnize will format output text.
type Config struct {
	// The string by which the lines of input will be split.
	Delim string

	// The string by which columns of output will be separated.
	Glue string

	// The string by which columns of output will be prefixed.
	Prefix string

	// A replacement string to replace empty fields.
	Empty string

	// NoTrim disables automatic trimming of inputs.
	NoTrim bool
}

// DefaultConfig returns a *Config with default values.
func DefaultConfig() *Config {
	return &Config{
		Delim:  "|",
		Glue:   "  ",
		Prefix: "",
		Empty:  "",
		NoTrim: false,
	}
}

// MergeConfig merges two config objects together and returns the resulting
// configuration. Values from the right take precedence over the left side.
func MergeConfig(a, b *Config) *Config {
	// Return quickly if either side was nil
	if a == nil {
		return b
	}
	if b == nil {
		return a
	}

	var result Config = *a

	if b.Delim != "" {
		result.Delim = b.Delim
	}
	if b.Glue != "" {
		result.Glue = b.Glue
	}
	if b.Prefix != "" {
		result.Prefix = b.Prefix
	}
	if b.Empty != "" {
		result.Empty = b.Empty
	}
	if b.NoTrim {
		result.NoTrim = true
	}

	return &result
}

func getFirstWords(str string, lenght int) string {
	result := ""
	currentLen := 0
	parts := strings.Split(str, " ")
	for _, p := range parts {
		if currentLen + len(p) >= lenght {
			break
		}

		result += p + " "
		currentLen += len(p) + 1
	}

	return result
}

func stringFormatElems(c *Config, widths []int, elems []interface{}) string {

	columns := len(elems)

	// Create the buffer with an estimate of the length
	buf := bytes.NewBuffer(make([]byte, 0, (6+len(c.Glue))*columns))

	// Start with the prefix, if any was given. The buffer will not return an
	// error so it does not need to be handled
	buf.WriteString(c.Prefix)

	// Create the format string from the discovered widths
	for i := 0; i < columns && i < len(widths); i++ {
		if i == columns-1 {
			str := elems[i].(string)
			// if string is too long, slipt it
			first := getFirstWords(str, widths[i])
			if len(str) > widths[i] && len(first) > 0 {
				fmt.Fprintf(buf, "%s\n", first)
				elemX := make([]interface{}, columns)
				for i, _ := range elemX {
					elemX[i] = ""
				}
				// in case there are still more pieces
				if len(str) != len(first) {
					elemX[columns-1] = str[len(first)-1:]
				} else {
					elemX[columns-1] = str
				}
				buf.WriteString(stringFormatElems(c, widths, elemX))
			} else {
				fmt.Fprintf(buf, "%s\n", str)
			}
		} else {
			fmt.Fprintf(buf, "%-*s%s", widths[i], elems[i], c.Glue)
		}
	}
	return buf.String()
}

// elementsFromLine returns a list of elements, each representing a single
// item which will belong to a column of output.
func elementsFromLine(config *Config, line []string) []interface{} {
	//separated := strings.Split(line, config.Delim)
	//elements := make([]interface{}, len(separated))
	elements := make([]interface{}, len(line))
	for i, field := range line {
		value := field
		if !config.NoTrim {
			value = strings.TrimSpace(field)
		}

		// Apply the empty value, if configured.
		if value == "" && config.Empty != "" {
			value = config.Empty
		}
		elements[i] = value
	}
	return elements
}

// runeLen calculates the number of visible "characters" in a string
func runeLen(s string) int {
	l := 0
	for _ = range s {
		l++
	}
	return l
}

// widthsFromLines examines a list of strings and determines how wide each
// column should be considering all of the elements that need to be printed
// within it.
func widthsFromLines(config *Config, lines [][]string, max int) []int {
	widths := make([]int, 0, 8)

	for _, line := range lines {
		elems := elementsFromLine(config, line)
		for i := 0; i < len(elems); i++ {
			l := runeLen(elems[i].(string))
			if len(widths) <= i {
				widths = append(widths, l)
			} else if widths[i] < l {
				widths[i] = l
			}
		}
	}

	t := 0
	for _, i := range widths {
		t += i
	}

	if t > max {
		widths[len(widths) - 1] -= (t - max)
	}

	return widths
}

// Format is the public-facing interface that takes a list of strings and
// returns nicely aligned column-formatted text.
func Format(lines [][]string, config *Config) string {
	conf := MergeConfig(DefaultConfig(), config)
	consoleSize := 80
	if goterm.Width() < 80 {
		fmt.Print("Size of the terminal is too small, output could be missaligned.")
	} else {
		consoleSize = goterm.Width()
	}
	widths := widthsFromLines(conf, lines, consoleSize-5) // -5 is a hack

	// Estimate the buffer size
	glueSize := len(conf.Glue)
	var size int
	for _, w := range widths {
		size += w + glueSize
	}
	size *= len(lines)

	// Create the buffer
	buf := bytes.NewBuffer(make([]byte, 0, size))

	// Create the formatted output using the format string
	for _, line := range lines {
		elems := elementsFromLine(conf, line)
		buf.WriteString(stringFormatElems(conf, widths, elems))
	}

	// Get the string result
	result := buf.String()

	// Remove trailing newline without removing leading/trailing space
	if n := len(result); n > 0 && result[n-1] == '\n' {
		result = result[:n-1]
	}

	return result
}

// SimpleFormat is a convenience function to format text with the defaults.
func SimpleFormat(lines [][]string) string {
	return Format(lines, nil)
}

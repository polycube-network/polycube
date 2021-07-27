module github.com/polycube-network/polycube/src/polycubectl

go 1.13

replace github.com/Jeffail/gabs2 => ./vendor/github.com/Jeffail/gabs2

replace github.com/ryanuber/columnize => ./vendor/github.com/ryanuber/columnize

require (
	github.com/BurntSushi/toml v0.3.1 // indirect
	github.com/Jeffail/gabs v1.4.0
	github.com/Jeffail/gabs2 v0.0.0
	github.com/buger/goterm v0.0.0-20200322175922-2f3e71b85129 // indirect
	github.com/ghodss/yaml v1.0.0
	github.com/iancoleman/orderedmap v0.2.0
	github.com/natefinch/lumberjack v2.0.0+incompatible
	github.com/ryanuber/columnize v2.1.2+incompatible
	golang.org/x/sys v0.0.0-20210317091845-390168757d9c // indirect
	gopkg.in/natefinch/lumberjack.v2 v2.0.0 // indirect
	gopkg.in/yaml.v2 v2.4.0
)

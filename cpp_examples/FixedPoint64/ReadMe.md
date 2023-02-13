swig 4.0.1/2 is ok, newer version has API break change.

4.0.2: 
`static Fix64::FromRaw` -> `Fix64.Fix64_FromRaw(..)`

4.1.x:
`static Fix64::FromRaw` -> `Fix64.FromRaw(..)`

swig doesn't provide compatibility between versions. see https://www.swig.org/Doc4.0/Preface.html#Preface_nn9

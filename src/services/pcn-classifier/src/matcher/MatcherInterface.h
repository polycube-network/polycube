/*
 * Copyright 2020 The Polycube Authors
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


#pragma once


#include "../Classifier.h"


class MatcherInterface {
 public:
  virtual MatchingField getField() = 0;
  virtual void initBitvector(uint32_t size) = 0;
  virtual void appendWildcardBit() = 0;
  virtual void loadTable(int prog_index, ProgramType direction) = 0;
  virtual std::string getTableCode()  = 0;
  virtual std::string getMatchingCode() = 0;
  virtual bool isActive() = 0;
};
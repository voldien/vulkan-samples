/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Valdemar Lindberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */
#pragma once
#include "Window.h"
#include <VulkanCore.h>

/**
 * @brief
 *
 */
class FVDECLSPEC IVKWindow : public fragcore::Window {
  public:
	virtual int x() const noexcept = 0;
	virtual int y() const noexcept = 0;

	void setUserData(void *userData) noexcept { this->userData = userData; }

	virtual VkSurfaceKHR createSurface(const std::shared_ptr<fvkcore::VulkanCore> &instance) = 0;

  public:
	~IVKWindow() override = default;

  protected:
	void *userData = nullptr;
};
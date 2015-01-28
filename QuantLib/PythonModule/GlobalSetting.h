#pragma once

static const int HistoricalVolDays = 120;
static const int HistoricalCorrWeeks = 52;

static const char* DB_SERVER_NAME = "fdos-mt";
static const char* DB_ID = "root";
static const char* DB_PASSWORD = "3450";

static const Real DefaultFXVol = 0.15;
static const Real DefaultFXCorr = -0.9;

static const Real DefaultCouponEpsilon = 0.001;
static const int DefaultCouponIteration = 10;

static const BigInteger BleedNextDayTime = 2 * 3600;

static const int LocalVolGridX = 100;
static const int LocalVolGridT = 100;
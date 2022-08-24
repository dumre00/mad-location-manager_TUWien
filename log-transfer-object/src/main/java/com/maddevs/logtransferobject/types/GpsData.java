package com.maddevs.logtransferobject.types;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonTypeName;
import com.maddevs.logtransferobject.Log;
import lombok.Data;
import lombok.experimental.SuperBuilder;
import lombok.extern.jackson.Jacksonized;

import java.math.BigDecimal;

@Jacksonized
@SuperBuilder
@JsonIgnoreProperties(ignoreUnknown = true)
@JsonTypeName("gps")
@Data
public class GpsData extends Log {
    private static final long serialVersionUID = 1L;
    // LogMessageType.GPS_DATA

    private BigDecimal lat;
    private BigDecimal lon;
    private BigDecimal alt;
    private BigDecimal hdop;
    private BigDecimal speed;
    private BigDecimal bearing;

    @Override
    public String toRawString() {
        return String.format("%s GPS : pos lat=%s lon=%s alt=%s hdop=%s speed=%s bearing=%s",
                getTimestamp(),
                lat,
                lon,
                alt,
                hdop,
                speed,
                bearing);
    }
}
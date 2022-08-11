package com.maddevs.logtransferobject.types;

import com.maddevs.logtransferobject.Log;
import com.maddevs.logtransferobject.LogMessageType;
import lombok.AccessLevel;
import lombok.Builder;
import lombok.Data;
import lombok.NoArgsConstructor;

import java.math.BigDecimal;

@Data
@Builder
public class KalmanPredict implements Log {
    private static final long serialVersionUID = 2L;

    private BigDecimal absEastAcceleration;
    private BigDecimal absNorthAcceleration;
    private BigDecimal absUpAcceleration;


    @Override
    public LogMessageType getLogMessageType() {
        return LogMessageType.KALMAN_PREDICT;
    }
}

//
//  ErrorMessage                                                           
//  ------------                                                           
//                                                                         
//  Both the parser generator and the parser can detect and return errors  
//  in this form. We only create errors on the C++ side but we provide the 
//  ability to pull them all to the python side for processing.            
//

package hoshi;

import java.lang.*;
import java.util.*;

public class ErrorMessage {

    private static final Initializer initializer = Initializer.getInitializer();

    private ErrorType type;
    private String tag;
    private int severity;
    private long location;
    private long lineNum;
    private long columnNum;
    private String sourceLine;
    private String shortMessage;
    private String longMessage;
    private String string;

    public ErrorType getType() {
        return type;
    }

    public void setType(ErrorType type) {
        this.type = type;
    }

    public String getTag() {
        return tag;
    }

    public void setTag(String tag) {
        this.tag = tag;
    }

    public int getSeverity() {
        return severity;
    }

    public void setSeverity(int severity) {
        this.severity = severity;
    }

    public long getLocation() {
        return location;
    }

    public void setLocation(long location) {
        this.location = location;
    }

    public long getLineNum() {
        return lineNum;
    }

    public void setLineNum(long lineNum) {
        this.lineNum = lineNum;
    }

    public long getColumnNum() {
        return columnNum;
    }

    public void setColumnNum(long columnNum) {
        this.columnNum = columnNum;
    }

    public String getSourceLine() {
        return sourceLine;
    }

    public void setSourceLine(String sourceLine) {
        this.sourceLine = sourceLine;
    }

    public String getShortMessage() {
        return shortMessage;
    }

    public void setShortMessage(String shortMessage) {
        this.shortMessage = shortMessage;
    }

    public String getLongMessage() {
        return longMessage;
    }

    public void setLongMessage(String longMessage) {
        this.longMessage = longMessage;
    }

    public String getString() {
        return string;
    }

    public void setString(String string) {
        this.string = string;
    }

}

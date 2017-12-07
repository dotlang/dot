; ModuleID = 'test2'
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [19 x i8] c"Assertion failed!\0A\00"

declare void @abort()
declare i32 @puts(i8* nocapture) nounwind

define i64 @bool_to_int(i1) {
entry:
    %is_true = icmp eq i1 %0, true
    %bool_to_int = select i1 %is_true, i64 1, i64 0
    ret i64 %bool_to_int
}

define i64 @float_to_int(double) {
entry:
    %float_to_int = fptosi double %0 to i64
    ret i64 %float_to_int
}

define i64 @char_to_int(i8) {
entry:
    %char_to_int = zext i8 %0 to i64
    ret i64 %char_to_int
}

define i64 @assert(i1) {
entry:
    %assert_result = icmp eq i1 %0, true
    br i1 %assert_result, label %assert_successful, label %assert_failed

    assert_successful:                                ; preds = %entry
    ret i64 1

    assert_failed:                                    ; preds = %entry
    %cast210 = getelementptr [19 x i8], [19 x i8]* @.str, i64 0, i64 0
    call i32 @puts(i8* %cast210)
    call void @abort()
    ret i64 0
}

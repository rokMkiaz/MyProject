
CREATE PROC [dbo].[gp_Save_WshingData]

	   @charunique		int,
	   @NecessaryTitle int,
	   @StoneIndex int,
	   @CSVStatIndex1 smallint,
	   @CSVStatIndex2 smallint,
	   @CSVStatIndex3 smallint,
	   @NextFlag smallint,
	   @StoneGrade tinyint,
	   @LockUse tinyint,
	   @ChangeCount int
AS
BEGIN
	if not exists (SELECT  * from [TWishing] WHERE  [charUnique]=@charunique and @NecessaryTitle = [NecessaryTitle] and @StoneIndex = StoneIndex)
	begin
			INSERT [TWishing]	([charunique] ,[NecessaryTitle] ,[StoneIndex],[CSVStatIndex1],[CSVStatIndex2] ,[CSVStatIndex3],[NextFlag],[StoneGrade],[LockUse],[Count] )
			VALUES				(@charunique	,@NecessaryTitle ,@StoneIndex ,@CSVStatIndex1 ,@CSVStatIndex2 ,@CSVStatIndex3 ,@NextFlag ,@StoneGrade ,@LockUse ,@ChangeCount )
	end
	else
	begin
		UPDATE [TWishing]
		set
	   [CSVStatIndex1]= @CSVStatIndex1,
	   [CSVStatIndex2]= @CSVStatIndex2,
	   [CSVStatIndex3]= @CSVStatIndex3,
	   [NextFlag]= @NextFlag	,
	   [StoneGrade] = @StoneGrade	,
	   [LockUse] = @LockUse		,
	   [Count] = @ChangeCount	
		where [charUnique]=@charunique
		and   [NecessaryTitle] = @NecessaryTitle
		and	  StoneIndex = @StoneIndex 

	end
END
go


CREATE PROC [dbo].[gp_Load_WshingData]

	   @charunique		int
AS
BEGIN
	select 	 		
			NecessaryTitle,
			StoneIndex,
			CSVStatIndex1,
			CSVStatIndex2,
			CSVStatIndex3,
			NextFlag,
			StoneGrade,
			LockUse,
			[Count]
	FROM [TWishing]
	WHERE charunique=@charunique
	Order by NecessaryTitle asc, StoneIndex asc
END
